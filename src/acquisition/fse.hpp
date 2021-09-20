#pragma once

#include "acquisition_result.hpp"
#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../matched_filter/matched_filter.hpp"
#include "../matched_filter/ipp_matched_filter.hpp"
#include "../math/ipp_abs.hpp"
#include "../math/ipp_dft.hpp"
#include "../math/ipp_max_index.hpp"
#include "../math/ipp_reshape_and_sum.hpp"
#include "../math/ipp_mean_stddev.hpp"
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
		static inline double peak_threshold = 3.5;

		using MixerType = Mixer<IppMixer>;
		using MatchedFilterType = MatchedFilter<IppMatchedFilter>;
		using AbsType = Abs<IppAbs>;
		using ReshapeAndSumType = ReshapeAndSum<IppReshapeAndSum>;
		using MaxIndexType = MaxIndex<IppMaxIndex>;
		using MeanStdDevType = MeanStdDev<IppMeanStdDev>;

		void InitSatellites() {
			gps_sv.resize(ugsdr::gps_sv_count);
			std::iota(gps_sv.begin(), gps_sv.end(), 0);
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			std::iota(gln_sv.begin(), gln_sv.end(), ugsdr::gln_min_frequency);
		}

		template <typename T>
		auto RepeatCodeNTimes(std::vector<T> code) {
			auto ms_size = code.size();
			for (std::size_t i = 1; i < ms_to_process; ++i)
				code.insert(code.end(), code.begin(), code.begin() + ms_size);
			
			return code;
		}

		template <typename T, bool coherent = true>
		auto GetOneMsPeak(const std::vector<std::complex<T>>& signal) {
			const auto samples_per_ms = static_cast<std::size_t>(signal_parameters.GetSamplingRate() / 1e3);
			
			if constexpr (coherent) {
				const auto abs_value = AbsType::Transform(signal);
				auto one_ms_peak = ReshapeAndSumType::Transform(abs_value, samples_per_ms);
				return one_ms_peak;
			}
			else {
				const auto one_ms_peak = ReshapeAndSumType::Transform(signal, samples_per_ms);
				const auto abs_peak = AbsType::Transform(one_ms_peak);
				return abs_peak;
			}
		}

		void ProcessBpsk(const std::vector<std::complex<UnderlyingType>>& signal, const std::vector<UnderlyingType>& code, std::int32_t sv, double intermediate_frequency, std::vector<AcquisitionResult>& dst) {
			decltype(GetOneMsPeak(MatchedFilterType::Filter(MixerType::Translate(signal, signal_parameters.GetSamplingRate(), 0.0), code))) output_peak;
			AcquisitionResult tmp, max_result;
			for (double doppler_frequency = intermediate_frequency - doppler_range;
						doppler_frequency <= intermediate_frequency + doppler_range;
						doppler_frequency += doppler_step) {
				const auto translated_signal = MixerType::Translate(signal, signal_parameters.GetSamplingRate(), -doppler_frequency);
				auto matched_output = MatchedFilterType::Filter(translated_signal, code);
				auto peak_one_ms = GetOneMsPeak(matched_output);

				auto max_index = MaxIndexType::Transform(peak_one_ms);
				auto mean_sigma = MeanStdDevType::Calculate(peak_one_ms);
				tmp.level = max_index.value;
				tmp.sigma = mean_sigma.sigma + mean_sigma.mean;
				tmp.code_offset = static_cast<double>(max_index.index);
				tmp.doppler = doppler_frequency;

				if (max_result < tmp) {
					max_result = tmp;
					output_peak = std::move(peak_one_ms);
				}
			}
			max_result.intermediate_frequency = intermediate_frequency;
			max_result.sv_number = sv;
			if (max_result.GetSnr() > peak_threshold) {
				dst.push_back(max_result);
				ugsdr::Add(L"Satellite " + std::to_wstring(sv), output_peak);
			}
		}
		
		void ProcessGps(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) {
			auto intermediate_frequency = -(signal_parameters.GetCentralFrequency() - 1575.42e6);
			//gps_sv.resize(2);

			for(auto sv : gps_sv) {
				const auto code = Upsampler<SequentialUpsampler>::Resample(RepeatCodeNTimes(Codegen<GpsL1Ca>::Get<UnderlyingType>(sv)),
					static_cast<std::size_t>(ms_to_process * signal_parameters.GetSamplingRate() / 1e3));
				
				ProcessBpsk(signal, code, sv, intermediate_frequency, dst);
			}
		}

		void ProcessGlonass(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) {
			const auto code = Upsampler<SequentialUpsampler>::Resample(RepeatCodeNTimes(Codegen<GlonassOf>::Get<UnderlyingType>(0)),
				static_cast<std::size_t>(ms_to_process * signal_parameters.GetSamplingRate() / 1e3));
			for (auto& litera_number : gln_sv) {
				auto intermediate_frequency = -(signal_parameters.GetCentralFrequency() - (1602e6 + litera_number * 0.5625e6));

				ProcessBpsk(signal, code, litera_number, intermediate_frequency, dst);
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
