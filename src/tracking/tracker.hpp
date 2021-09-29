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

#include <map>
#include <span>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	struct Codes final {
	private:
		using UpsamplerType = Upsampler<SequentialUpsampler>;

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
				codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Gps>::Get<UnderlyingType>(0), 3),
					static_cast<std::size_t>(3 * sampling_rate / 1e3));
			}

			auto sv = Sv{ 0, System::Glonass };
			codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Glonass>::Get<UnderlyingType>(0), 3),
				static_cast<std::size_t>(3 * sampling_rate / 1e3));
		}

		const auto& GetMap() const {
			return codes;
		}
	};


	template <typename UnderlyingType>
	class Tracker final {
		SignalParametersBase<UnderlyingType>& signal_parameters;
		const std::vector<AcquisitionResult>& acquisition_results;

		Codes<UnderlyingType> codes;
		const std::size_t samples_per_ms = 0;
		std::vector<TrackingParameters> tracking_parameters;

		using CorrelatorType = IppCorrelator;
		using MatchedFilterType = IppMatchedFilter;
		using MixerType = Mixer<IppMixer>;
		
		void InitTrackingParameters() {
			for (const auto& el : acquisition_results)
				tracking_parameters.emplace_back(el, signal_parameters.GetSamplingRate());
		}
		
		template <typename T>
		void TrackSingleSatellite(TrackingParameters& parameters, const T& signal) {
			const auto& code_map = codes.GetMap();
			const auto& full_code = code_map.at(parameters.sv);

			auto full_phase = static_cast<std::size_t>(samples_per_ms + parameters.code_phase * 0);
			const auto translated_signal = MixerType::Translate(signal, parameters.sampling_rate, parameters.carrier_frequency, parameters.carrier_phase); // <= increment phase!
			const auto current_code = std::span(full_code.begin() + full_phase, samples_per_ms);
			const auto current_code_v = std::vector(full_code.begin() + full_phase, full_code.begin() + full_phase + samples_per_ms);

			ugsdr::Add(MatchedFilter<IppMatchedFilter>::Filter(translated_signal, current_code_v));
			
			auto prompt = CorrelatorType::Correlate(translated_signal, current_code);
			parameters.prompt.push_back(prompt);
		}
		
	public:
		Tracker(SignalParametersBase<UnderlyingType>& signal_params, 
			const std::vector<AcquisitionResult>& acquisition_dst) :	signal_parameters(signal_params), acquisition_results(acquisition_dst), 
																		codes(signal_parameters.GetSamplingRate()), 
																		samples_per_ms(static_cast<std::size_t>(signal_parameters.GetSamplingRate() / 1e3)) {
			InitTrackingParameters();
		}

		void Track(std::size_t epochs_to_process) {
			auto timer = boost::timer::progress_display(static_cast<unsigned long>(epochs_to_process));
			
			for(std::size_t i = 0; i < epochs_to_process; ++i, ++timer) {
				auto current_signal_ms = signal_parameters.GetOneMs(i);

				for (auto& current_tracking_parameters : tracking_parameters)
					TrackSingleSatellite(current_tracking_parameters, current_signal_ms);
				return;
			}

			for(auto&el:tracking_parameters) {
				ugsdr::Add(L"Tracking result", el.prompt);
			}
		}
	};
}
