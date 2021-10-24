#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../dfe/dfe.hpp"
#include "../resample/upsampler.hpp"
#include "../correlator/af_correlator.hpp"
#include "../correlator/ipp_correlator.hpp"
#include "tracking_parameters.hpp"

#include "../matched_filter/ipp_matched_filter.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../mixer/table_mixer.hpp"

#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

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

		Codes(const DigitalFrontend<UnderlyingType>& digital_frontend) {
			auto gps_sampling_rate = digital_frontend.GetSamplingRate(Signal::GpsCoarseAcquisition_L1);
			
			for (std::int32_t i = 0; i < gps_sv_count; ++i) {
				auto sv = Sv{ i, System::Gps};
				codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Gps>::Get<UnderlyingType>(i), 3),
					static_cast<std::size_t>(3 * gps_sampling_rate / 1e3));
			}

			auto gln_sampling_rate = digital_frontend.GetSamplingRate(Signal::GlonassCivilFdma_L1);
			auto sv = glonass_sv;
			codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Glonass>::Get<UnderlyingType>(0), 3),
				static_cast<std::size_t>(3 * gln_sampling_rate / 1e3));
		}

		const auto& GetCode(Sv sv) const {
			if (sv.system == System::Glonass)
				sv = glonass_sv;
			
			return codes.at(sv);
		}
	};

	template <typename UnderlyingType>
	class Tracker final {
		DigitalFrontend<UnderlyingType>& digital_frontend;
		const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_results;

		Codes<UnderlyingType> codes;
		std::vector<TrackingParameters<UnderlyingType>> tracking_parameters;

		using CorrelatorType = IppCorrelator;
		using MatchedFilterType = IppMatchedFilter;
		using MixerType = Mixer<IppMixer>;
		
		void InitTrackingParameters() {
			for (const auto& el : acquisition_results)
				tracking_parameters.emplace_back(el, digital_frontend);
		}

		template <typename T>
		auto AddWithPhase(std::complex<T> lhs, std::complex<T> rhs, double relative_phase) {
			if (relative_phase > 0.5)
				std::swap(lhs, rhs);

			const auto weighted_lhs = lhs.real() / relative_phase;
			const auto weighted_rhs = rhs.real() / (1.0 - relative_phase);
			if (std::abs(weighted_lhs + weighted_rhs) > std::abs(weighted_lhs))
				return lhs + rhs;
			return lhs - rhs;
		}

		template <typename T1, typename T2>
		auto Correlate(const T1& translated_signal, const T2& full_code, std::size_t samples_per_ms, std::pair<double, std::complex<UnderlyingType>>& code_phase_and_output) {
			auto full_phase = static_cast<std::size_t>(std::ceil(samples_per_ms + code_phase_and_output.first));
			const auto current_code = std::span(full_code.begin() + full_phase, samples_per_ms);
			code_phase_and_output.second = CorrelatorType::Correlate(std::span(translated_signal), current_code);
		}

		template <typename T1, typename T2>
		auto CorrelateSplit(const T1& translated_signal, const T2& full_code, std::size_t samples_per_ms, std::pair<double, std::complex<UnderlyingType>>& code_phase_and_output) {
			if (code_phase_and_output.first < 0)
				code_phase_and_output.first += samples_per_ms;

			auto first_batch_length = static_cast<std::size_t>(std::ceil(samples_per_ms - code_phase_and_output.first));
			auto second_batch_length = samples_per_ms - first_batch_length;

			auto first_batch_phase = static_cast<std::size_t>(std::ceil(samples_per_ms + code_phase_and_output.first));
			auto second_batch_phase = first_batch_phase + first_batch_length;

			auto first = CorrelatorType::Correlate(std::span(translated_signal.begin(), first_batch_length), std::span(full_code.begin() + first_batch_phase, first_batch_length));
			auto second = CorrelatorType::Correlate(std::span(translated_signal.begin() + first_batch_length, second_batch_length), std::span(full_code.begin() + second_batch_phase, second_batch_length));
			

			code_phase_and_output.second = AddWithPhase(first, second, code_phase_and_output.first / samples_per_ms);
		}

		auto GetEpl(TrackingParameters<UnderlyingType>& parameters, double spacing_chips) {
			auto& translated_signal = parameters.translated_signal;
			MixerType::Translate(translated_signal, parameters.sampling_rate, -parameters.carrier_frequency, -parameters.carrier_phase);
		
			auto phase_mod = std::fmod(parameters.carrier_frequency / 1000.0, 1.0);
			if (phase_mod < 0)
				phase_mod += 1;
			parameters.carrier_phase += 2 * std::numbers::pi_v<double> * phase_mod;
			
			const auto& full_code = codes.GetCode(parameters.sv);

			auto spacing_offset = parameters.GetSamplesPerChip() * spacing_chips;

			auto output_array = std::array{
				std::make_pair(parameters.code_phase + spacing_offset, std::complex<UnderlyingType>{}),
				std::make_pair(parameters.code_phase, std::complex<UnderlyingType>{}),
				std::make_pair(parameters.code_phase - spacing_offset, std::complex<UnderlyingType>{}),
			};

			std::for_each(std::execution::par_unseq, output_array.begin(), output_array.end(), [&translated_signal, &full_code, samples_per_ms = static_cast<std::size_t>(parameters.sampling_rate / 1e3), this](auto& pair) {
				CorrelateSplit(translated_signal, full_code, samples_per_ms, pair);
			});

			return std::make_tuple(output_array[0].second, output_array[1].second, output_array[2].second);
		}


		static auto GetCopyWrapper() {
			static auto copy_wrapper = plusifier::FunctionWrapper(
				ippsCopy_32fc, ippsCopy_64fc
			);

			return copy_wrapper;
		}

		void TrackSingleSatellite(TrackingParameters<UnderlyingType>& parameters, const SignalEpoch<UnderlyingType>& signal_epoch) {
			const auto& signal = signal_epoch.GetSubband(parameters.signal_type);

			auto copy_wrapper = GetCopyWrapper();
						
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			copy_wrapper(reinterpret_cast<const IppType*>(signal.data()), reinterpret_cast<IppType*>(parameters.translated_signal.data()), static_cast<int>(signal.size()));

			
			auto [early, prompt, late] = GetEpl(parameters, 0.25);
			parameters.early.push_back(early);
			parameters.prompt.push_back(prompt);
			parameters.late.push_back(late);

			parameters.Pll(prompt);
			parameters.Dll(early, late);
		}
		
	public:
		Tracker(DigitalFrontend<UnderlyingType>& dfe, 
			const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_dst) :	digital_frontend(dfe), acquisition_results(acquisition_dst),
																						codes(digital_frontend) {
			InitTrackingParameters();
		}

		void Track(std::size_t epochs_to_process) {
			auto timer = boost::timer::progress_display(static_cast<unsigned long>(epochs_to_process));

			SignalEpoch<UnderlyingType> current_signal_ms;
			for (std::size_t i = 0; i < epochs_to_process; ++i, ++timer) {
				digital_frontend.GetEpoch(i, current_signal_ms);

				std::for_each(std::execution::par_unseq, tracking_parameters.begin(), tracking_parameters.end(),
					[&current_signal_ms, this](auto& current_tracking_parameters) {
						TrackSingleSatellite(current_tracking_parameters, current_signal_ms);
					});
			}
		}

		void Plot() const {
			for (auto& el : tracking_parameters) {
				//ugsdr::Add(L"Early tracking result", el.early);
				ugsdr::Add(L"Prompt tracking result", el.prompt);
				//ugsdr::Add(L"Late tracking result", el.late);
				//return;
			}			
		}

		const auto& GetTrackingParameters() const {
			return tracking_parameters;
		}
	};
}
