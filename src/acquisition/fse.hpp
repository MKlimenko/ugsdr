#pragma once

#include "acquisition_result.hpp"
#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../matched_filter/matched_filter.hpp"
#include "../matched_filter/ipp_matched_filter.hpp"
#include "../math/ipp_dft.hpp"
#include "../math/upsample.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../prn_codes/codegen.hpp"
#include "../prn_codes/GpsL1Ca.hpp"
#include "../prn_codes/GlonassOf.hpp"


#include <numeric>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class FastSearchEngineBase {
	private:
		constexpr static std::size_t ms_to_process = 5;
		
		SignalParametersBase<UnderlyingType>& signal_parameters;
		double doppler_range = 5e3;
		double doppler_step = 20;
		std::vector<std::int32_t> gps_sv;
		std::vector<std::int32_t> gln_sv;
		
		using MixerType = Mixer<IppMixer>;
		using MatchedFilterType = MatchedFilter<IppMatchedFilter>;

		void InitSatellites() {
			gps_sv.resize(ugsdr::gps_sv_count);
			std::iota(gps_sv.begin(), gps_sv.end(), 0);
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			std::iota(gln_sv.begin(), gln_sv.end(), ugsdr::gln_min_frequency);
		}

		void ProcessBpsk(const std::vector<std::complex<UnderlyingType>>& translated_signal, std::vector<AcquisitionResult>& dst) {
			
		}

		template <typename T>
		auto RepeatCodeNTimes(std::vector<T> code) {
			auto ms_size = code.size();
			for (std::size_t i = 1; i < ms_to_process; ++i)
				code.insert(code.end(), code.begin(), code.begin() + ms_size);
			
			return code;
		}
		
		void ProcessGps(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) {
			auto intermediate_frequency = signal_parameters.GetCentralFrequency() - 1575.42e6;
			gps_sv.resize(2);

			for(auto sv : gps_sv) {
				const auto code = Upsampler<SequentialUpsampler>::Resample(RepeatCodeNTimes(Codegen<GpsL1Ca>::Get<UnderlyingType>(sv)),
					static_cast<std::size_t>(ms_to_process * signal_parameters.GetSamplingRate() / 1e3));

				for (double doppler_frequency = intermediate_frequency - doppler_range; 
							doppler_frequency <= intermediate_frequency + doppler_range; 
							doppler_frequency += doppler_step) {
					const auto translated_signal = MixerType::Translate(signal, signal_parameters.GetSamplingRate(), doppler_frequency);
					ugsdr::Add(IppDft::Transform(translated_signal));
					return;
					//auto matched_output = MatchedFilterType::Filter(translated_signal, code);

				}
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
