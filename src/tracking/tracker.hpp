#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../resample/upsampler.hpp"
#include "../correlator/af_correlator.hpp"
#include "../correlator/ipp_correlator.hpp"
#include "tracking_parameters.hpp"

#include "../matched_filter/ipp_matched_filter.hpp"
#include "../mixer/ipp_mixer.hpp"

#include "boost/timer/progress_display.hpp"

#include <execution>
#include <map>
#include <numbers>
#include <span>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	struct Codes final {
	private:
		using UpsamplerType = Upsampler<SequentialUpsampler>;

		constexpr static inline Sv glonass_sv = Sv{ 0, System::Glonass };
		
		template <typename T>
		auto RepeatCodeNTimes(std::vector<T> code, std::size_t repeats) {
			auto ms_size = code.size();
			for (std::size_t i = 1; i < repeats; ++i)
				code.insert(code.end(), code.begin(), code.begin() + ms_size);

			return code;
		}

		using MapType = std::map<Sv, std::vector<UnderlyingType>>;

	public:
		MapType codes;

		Codes(double sampling_rate) {
			for (std::int32_t i = 0; i < gps_sv_count; ++i) {
				auto sv = Sv{ i, System::Gps};
				codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Gps>::Get<UnderlyingType>(i), 3),
					static_cast<std::size_t>(3 * sampling_rate / 1e3));
			}

			auto sv = glonass_sv;
			codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Glonass>::Get<UnderlyingType>(0), 3),
				static_cast<std::size_t>(3 * sampling_rate / 1e3));
		}

		const auto& GetCode(Sv sv) const {
			if (sv.system == System::Glonass)
				sv = glonass_sv;
			
			return codes.at(sv);
		}
	};


	template <typename UnderlyingType>
	class Tracker final {
		SignalParametersBase<UnderlyingType>& signal_parameters;
		const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_results;

		Codes<UnderlyingType> codes;
		const std::size_t samples_per_ms = 0;
		std::vector<TrackingParameters<UnderlyingType>> tracking_parameters;

		using CorrelatorType = IppCorrelator;
		using MatchedFilterType = IppMatchedFilter;
		using MixerType = Mixer<IppMixer>;
		
		void InitTrackingParameters() {
			for (const auto& el : acquisition_results)
				tracking_parameters.emplace_back(el, signal_parameters.GetSamplingRate(), signal_parameters.GetNumberOfEpochs());
		}

		template <typename T1, typename T2>
		auto Correlate(const T1& translated_signal, const T2& full_code, double code_phase) {
			auto full_phase = static_cast<std::size_t>(samples_per_ms + code_phase);
			const auto current_code = std::span(full_code.begin() + full_phase, samples_per_ms);
			return CorrelatorType::Correlate(translated_signal, current_code);
		}

		template <typename T1, typename T2>
		auto Correlate(const T1& translated_signal, const T2& full_code, std::pair<double, std::complex<UnderlyingType>>& code_phase_and_output) {
			auto full_phase = static_cast<std::size_t>(samples_per_ms + code_phase_and_output.first);
			const auto current_code = std::span(full_code.begin() + full_phase, samples_per_ms);
			code_phase_and_output.second = CorrelatorType::Correlate(translated_signal, current_code);
		}

		template <typename T>
		auto GetEpl(TrackingParameters<UnderlyingType>& parameters, const T& signal, double spacing_chips) {
			auto translated_signal = MixerType::Translate(signal, parameters.sampling_rate, -parameters.carrier_frequency, parameters.carrier_phase);
			parameters.carrier_phase += 2 * std::numbers::pi_v<double> *std::fmod(parameters.carrier_frequency / 1000.0, 1.0);
			
			const auto& full_code = codes.GetCode(parameters.sv);

			auto spacing_offset = parameters.GetSamplesPerChip() * spacing_chips;

			auto output_array = std::array{
				std::make_pair(parameters.code_phase - spacing_offset, std::complex<UnderlyingType>{}),
				std::make_pair(parameters.code_phase, std::complex<UnderlyingType>{}),
				std::make_pair(parameters.code_phase + spacing_offset, std::complex<UnderlyingType>{}),
			};
			
			//auto early_value = Correlate(translated_signal, full_code, parameters.code_phase - spacing_offset);
			//auto prompt_value = Correlate(translated_signal, full_code, parameters.code_phase);
			//auto late_value = Correlate(translated_signal, full_code, parameters.code_phase + spacing_offset);
			//return std::make_tuple(early_value, prompt_value, late_value);
			std::for_each(std::execution::par_unseq, output_array.begin(), output_array.end(), [&](auto&pair) {
				Correlate(translated_signal, full_code, pair);
			});

			return std::make_tuple(output_array[0].second, output_array[1].second, output_array[2].second);
		}

		template <typename T>
		void TrackSingleSatellite(TrackingParameters<UnderlyingType>& parameters, const T& signal) {
			auto [early, prompt, late] = GetEpl(parameters, signal, 0.5);
			parameters.early.push_back(early);
			parameters.prompt.push_back(prompt);
			parameters.late.push_back(late);
		}
		
	public:
		Tracker(SignalParametersBase<UnderlyingType>& signal_params, 
			const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_dst) :	signal_parameters(signal_params), acquisition_results(acquisition_dst),
																						codes(signal_parameters.GetSamplingRate()), 
																						samples_per_ms(static_cast<std::size_t>(signal_parameters.GetSamplingRate() / 1e3)) {
			InitTrackingParameters();
		}

		void Track(std::size_t epochs_to_process) {
			auto timer = boost::timer::progress_display(static_cast<unsigned long>(epochs_to_process));

			for (std::size_t i = 0; i < epochs_to_process; ++i, ++timer) {
				auto current_signal_ms = signal_parameters.GetOneMs(i);

				std::for_each(std::execution::par_unseq, tracking_parameters.begin(), tracking_parameters.end(),
					[&current_signal_ms, this](auto& current_tracking_parameters) {
						TrackSingleSatellite(current_tracking_parameters, current_signal_ms);
					});
			}

			for (auto& el : tracking_parameters) {
				ugsdr::Add(L"Early tracking result", el.early);
				ugsdr::Add(L"Prompt tracking result", el.prompt);
				ugsdr::Add(L"Late tracking result", el.late);
				return;
			}
		}
	};
}
