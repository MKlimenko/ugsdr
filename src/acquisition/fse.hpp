#pragma once

#include "acquisition_result.hpp"
#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../dfe/dfe.hpp"
#include "../matched_filter/matched_filter.hpp"
#include "../matched_filter/af_matched_filter.hpp"
#include "../matched_filter/ipp_matched_filter.hpp"
#include "../math/ipp_abs.hpp"
#include "../math/ipp_max_index.hpp"
#include "../math/ipp_reshape_and_sum.hpp"
#include "../math/ipp_mean_stddev.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../mixer/table_mixer.hpp"
#include "../prn_codes/codegen_wrapper.hpp"
#include "../resample/upsampler.hpp"
#include "../resample/ipp_resampler.hpp"

#include <cstring>
#include <functional>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class FastSearchEngineBase final {
	private:
		constexpr static std::size_t ms_to_process = 5;

		DigitalFrontend<UnderlyingType>& digital_frontend;
		double doppler_range = 5e3;
		double doppler_step = 20;
		std::vector<Sv> gps_sv;
		std::vector<Sv> gln_sv;
		constexpr static inline double peak_threshold = 3.5;
		constexpr static inline double acquisition_sampling_rate = 4e6;

		std::mutex m;

		using MixerType = IppMixer;
		using UpsamplerType = SequentialUpsampler;
		using MatchedFilterType = IppMatchedFilter;
		using AbsType = IppAbs;
		using ReshapeAndSumType = IppReshapeAndSum;
		using MaxIndexType = IppMaxIndex;
		using MeanStdDevType = IppMeanStdDev;
	
		void InitSatellites() {
			gps_sv.resize(ugsdr::gps_sv_count);
			for(std::size_t i = 0; i < gps_sv.size(); ++i) {
				gps_sv[i].system = System::Gps;
				gps_sv[i].id = static_cast<std::int32_t>(i);
			}
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			for (std::size_t i = 0; i < gln_sv.size(); ++i) {
				gln_sv[i].system = System::Glonass;
				gln_sv[i].id = ugsdr::gln_min_frequency + static_cast<std::int32_t>(i);
			}
		}

		template <typename T>
		auto RepeatCodeNTimes(std::vector<T> code, std::size_t repeats) {
			auto ms_size = code.size();
			for (std::size_t i = 1; i < repeats; ++i)
				code.insert(code.end(), code.begin(), code.begin() + ms_size);
			
			return code;
		}

		template <typename T, bool coherent = true>
		auto GetOneMsPeak(const std::vector<std::complex<T>>& signal, double new_sampling_rate) {
			const auto samples_per_ms = static_cast<std::size_t>(new_sampling_rate / 1e3);
			
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

		auto AdjustSamplingRate(double signal_sampling_rate) const {
			auto new_sampling_rate = signal_sampling_rate;
			auto delta = [](auto lhs, auto rhs) {
				return std::abs(lhs - rhs);
			};

			for (std::size_t i = 1; i < static_cast<std::size_t>(2 * signal_sampling_rate / acquisition_sampling_rate); ++i) {
				auto fs_candidate = signal_sampling_rate / i;
				if (std::fmod(fs_candidate / 1e3, 1) != 0.0)
					continue;
				
				if (delta(acquisition_sampling_rate, fs_candidate) > delta(acquisition_sampling_rate, new_sampling_rate))
					return new_sampling_rate;

				new_sampling_rate = fs_candidate;
			}
			
			return signal_sampling_rate;
		}

		void ProcessBpsk(const std::vector<std::complex<UnderlyingType>>& signal, const std::vector<UnderlyingType>& code,
			Sv sv, double signal_sampling_rate, double new_sampling_rate, double intermediate_frequency, 
			std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			AcquisitionResult<UnderlyingType> tmp, max_result;
			auto ratio = signal_sampling_rate / new_sampling_rate;

			auto code_spectrum = MatchedFilterType::PrepareCodeSpectrum(code);
			
			for (double doppler_frequency = -doppler_range;
						doppler_frequency <= doppler_range;
						doppler_frequency += doppler_step) {
				const auto translated_signal = MixerType::Translate(signal, new_sampling_rate, -doppler_frequency);
				auto matched_output = MatchedFilterType::FilterOptimized(translated_signal, code_spectrum);
				auto peak_one_ms = GetOneMsPeak(matched_output, new_sampling_rate);

				std::reverse(std::execution::par_unseq, peak_one_ms.begin(), peak_one_ms.end());
				
				auto max_index = MaxIndexType::Transform(peak_one_ms);
				auto mean_sigma = MeanStdDevType::Calculate(peak_one_ms);
				tmp.level = max_index.value;
				tmp.sigma = mean_sigma.sigma + mean_sigma.mean;
				tmp.code_offset = ratio * max_index.index;
				tmp.doppler = doppler_frequency + intermediate_frequency;

				if (max_result < tmp) {
					max_result = tmp;
					max_result.output_peak = std::move(peak_one_ms);
				}
			}
			max_result.intermediate_frequency = intermediate_frequency;
			max_result.sv_number = sv;
			if (max_result.GetSnr() > peak_threshold) {
				auto lock = std::unique_lock(m);
				dst.push_back(std::move(max_result));
			}
		}
		
		void ProcessGps(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::GpsCoarseAcquisition_L1);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::GpsCoarseAcquisition_L1);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::GpsCoarseAcquisition_L1);
			
			auto intermediate_frequency = -(central_frequency - 1575.42e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));

			std::for_each(std::execution::par_unseq, gps_sv.begin(), gps_sv.end(), [&](auto sv) {
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Gps>::Get<UnderlyingType>(static_cast<std::int32_t>(sv)), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
			
		}

		void ProcessGlonass(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::GlonassCivilFdma_L1);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::GlonassCivilFdma_L1);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::GlonassCivilFdma_L1);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);

			const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Glonass>::Get<UnderlyingType>(0), ms_to_process),
				static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

			std::for_each(std::execution::par_unseq, gln_sv.begin(), gln_sv.end(), [&](Sv litera_number) {
				auto intermediate_frequency = -(central_frequency - (1602e6 + static_cast<std::int32_t>(litera_number) * 0.5625e6));
				const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
				auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
					static_cast<std::size_t>(signal_sampling_rate));

				ProcessBpsk(downsampled_signal, code, litera_number, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
		}
		
	public:
		FastSearchEngineBase(DigitalFrontend<UnderlyingType>& dfe, double range, double step) :
																														digital_frontend(dfe),
																														doppler_range(range),
																														doppler_step(step) {
			InitSatellites();
		}

		auto Process(bool plot_results = false, std::size_t ms_offset = 0) {
			std::vector<AcquisitionResult<UnderlyingType>> dst;
			dst.reserve(gps_sv.size() + gln_sv.size());
			auto epoch_data = digital_frontend.GetSeveralEpochs(ms_offset, ms_to_process);

			if (plot_results) {
				if (digital_frontend.HasSignal(Signal::GpsCoarseAcquisition_L1))
					ugsdr::Add(L"GPS acquisition input signal", epoch_data.GetSubband(Signal::GpsCoarseAcquisition_L1), digital_frontend.GetSamplingRate(Signal::GpsCoarseAcquisition_L1));
				if (digital_frontend.HasSignal(Signal::GlonassCivilFdma_L1))
					ugsdr::Add(L"Glonass acquisition input signal", epoch_data.GetSubband(Signal::GlonassCivilFdma_L1), digital_frontend.GetSamplingRate(Signal::GlonassCivilFdma_L1));
			}
				
			if (digital_frontend.HasSignal(Signal::GpsCoarseAcquisition_L1))
				ProcessGps(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::GlonassCivilFdma_L1))
				ProcessGlonass(epoch_data, dst);
		
			std::sort(dst.begin(), dst.end(), [](auto& lhs, auto& rhs) {
				return lhs.sv_number < rhs.sv_number;
			});

			if (plot_results)
				for (auto& acquisition_result : dst)
					ugsdr::Add(L"Satellite " + std::to_wstring(static_cast<std::uint32_t>(acquisition_result.sv_number)), acquisition_result.output_peak);
		
			return dst;
		}
	};

	using FastSearchEngine = FastSearchEngineBase<float>;
}
