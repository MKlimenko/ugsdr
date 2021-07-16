#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../prn_codes/codegen.hpp"
#include "../prn_codes/GpsL1Ca.hpp"
#include "../math/upsample.hpp"
#include "acquisition_result.hpp"

#include <numeric>
#include <ranges>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class FastSearchEngineBase {
	private:
		constexpr static std::size_t ms_to_process = 5;
		
		SignalParametersBase<UnderlyingType>& signal_parameters;
		double doppler_range = 5e3;
		double doppler_step = 0;
		std::vector<std::int32_t> gps_sv;
		std::vector<std::int32_t> gln_sv;
		
		using MixerType = Mixer<IppMixer>;

		void InitSatellites() {
			gps_sv.resize(ugsdr::gps_sv_count);
			std::iota(gps_sv.begin(), gps_sv.end(), 0);
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			std::iota(gln_sv.begin(), gln_sv.end(), ugsdr::gln_min_frequency);
		}

		void ProcessBpsk(const std::vector<std::complex<UnderlyingType>>& translated_signal, std::vector<AcquisitionResult>& dst) {
			
		}
		
		void ProcessGps(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) {
			auto intermediate_frequency = signal_parameters.GetCentralFrequency() - 1575.42e6;
			auto translated_signal = MixerType::Translate(signal, signal_parameters.GetSamplingRate(), intermediate_frequency);
			ugsdr::Add(L"Translated GPS signal", translated_signal);
			gps_sv.resize(2);
			for(auto sv : gps_sv) {
				auto code = Upsampler<SequentialUpsampler>::Resample(Codegen<GpsL1Ca>::Get<UnderlyingType>(sv), static_cast<std::size_t>(signal_parameters.GetSamplingRate() / 1e3));
				
			}
			
		}

		void ProcessGlonass(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) {
			for (auto& litera_number : gln_sv) {
				auto intermediate_frequency = signal_parameters.GetCentralFrequency() - (1602e6 + litera_number * 0.5625e6);
				auto translated_signal = MixerType::Translate(signal, signal_parameters.GetSamplingRate(), intermediate_frequency);

				
			}
		}
		
	public:
		FastSearchEngineBase(SignalParametersBase<UnderlyingType>& signal_params, double range, double step) :
																														signal_parameters(signal_params),
																														doppler_range(range),
																														doppler_step(step) {
			InitSatellites();
		}

		auto Process(std::size_t ms_offset) {
			std::vector<AcquisitionResult> dst;
			auto signal = signal_parameters.GetSeveralMs(ms_offset, ms_to_process);

			ugsdr::Add(L"Acquisition input signal", signal, signal_parameters.GetSamplingRate());

			ProcessGps(signal, dst);
			ProcessGlonass(signal, dst);

			return dst;
		}
	};

	using FastSearchEngine = FastSearchEngineBase<std::int8_t>;
}