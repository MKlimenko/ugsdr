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
		constexpr static std::size_t ms_to_process = 4;

		DigitalFrontend<UnderlyingType>& digital_frontend;
		double doppler_range = 5e3;
		double doppler_step = 20;
		std::vector<Sv> gps_sv;
		std::vector<Sv> gln_sv;
		std::vector<Sv> galileo_sv;
		std::vector<Sv> beidou_sv;
		std::vector<Sv> navic_sv;
		std::vector<Sv> sbas_sv;
		std::vector<Sv> qzss_sv;
		constexpr static inline double peak_threshold = 3.5;
		constexpr static inline double acquisition_sampling_rate = 8.192e6;
		constexpr static inline double acquisition_sampling_rate_L5 = 20.46e6;

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
				gps_sv[i].signal = Signal::GpsCoarseAcquisition_L1;
			}
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			for (std::size_t i = 0; i < gln_sv.size(); ++i) {
				gln_sv[i].system = System::Glonass;
				gln_sv[i].id = ugsdr::gln_min_frequency + static_cast<std::int32_t>(i);
				gln_sv[i].signal = Signal::GlonassCivilFdma_L1;
			}
			galileo_sv.resize(galileo_sv_count);
			for (std::size_t i = 0; i < galileo_sv.size(); ++i) {
				galileo_sv[i].system = System::Galileo;
				galileo_sv[i].id = static_cast<std::int32_t>(i);
				galileo_sv[i].signal = Signal::Galileo_E1b;
			}
			beidou_sv.resize(ugsdr::beidou_sv_count);
			for (std::size_t i = 0; i < beidou_sv.size(); ++i) {
				beidou_sv[i].system = System::BeiDou;
				beidou_sv[i].id = static_cast<std::int32_t>(i);
				beidou_sv[i].signal = Signal::BeiDou_B1I;
			}
			navic_sv.resize(ugsdr::navic_sv_count);
			for (std::size_t i = 0; i < navic_sv.size(); ++i) {
				navic_sv[i].system = System::NavIC;
				navic_sv[i].id = static_cast<std::int32_t>(i);
				navic_sv[i].signal = Signal::NavIC_L5;
			}
			sbas_sv.resize(ugsdr::sbas_sv_count);
			for (std::size_t i = 0; i < sbas_sv.size(); ++i) {
				sbas_sv[i].system = System::Sbas;
				sbas_sv[i].id = static_cast<std::int32_t>(i + ugsdr::sbas_sv_offset);
				sbas_sv[i].signal = Signal::Sbas_L5Q;
			}
			qzss_sv.resize(ugsdr::qzss_sv_count);
			for (std::size_t i = 0; i < qzss_sv.size(); ++i) {
				qzss_sv[i].system = System::Qzss;
				qzss_sv[i].id = static_cast<std::int32_t>(i);
				qzss_sv[i].signal = Signal::QzssCoarseAcquisition_L1;
			}
		}

		template <bool reshape = true, bool coherent = true, typename T = int>
		auto GetOneMsPeak(const std::vector<std::complex<T>>& signal, double new_sampling_rate) {
			const auto samples_per_ms = static_cast<std::size_t>(new_sampling_rate / 1e3);
			
			if constexpr (!reshape) {
				return AbsType::Transform(signal);
			}
			else {
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
		}

		auto AdjustSamplingRate(double signal_sampling_rate, double target_sampling_rate = acquisition_sampling_rate) const {
			auto new_sampling_rate = signal_sampling_rate;
			auto delta = [](auto lhs, auto rhs) {
				return std::abs(lhs - rhs);
			};

			for (std::size_t i = 1; i < static_cast<std::size_t>(2 * signal_sampling_rate / target_sampling_rate); ++i) {
				auto fs_candidate = signal_sampling_rate / i;
				if (std::fmod(fs_candidate / 1e3, 1) != 0.0)
					continue;
				
				if (delta(target_sampling_rate, fs_candidate) > delta(target_sampling_rate, new_sampling_rate))
					return new_sampling_rate;

				new_sampling_rate = fs_candidate;
			}
			
			return signal_sampling_rate;
		}

		template <bool reshape = true, bool coherent = true>
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
				auto peak_one_ms = GetOneMsPeak<reshape, coherent>(matched_output, new_sampling_rate);
				
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
			if (!reshape || max_result.GetSnr() > peak_threshold) {
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
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::GpsCoarseAcquisition_L1>::Get<UnderlyingType>(sv.id), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
		}

		void ProcessGlonass(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::GlonassCivilFdma_L1);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::GlonassCivilFdma_L1);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::GlonassCivilFdma_L1);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);

			const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::GlonassCivilFdma_L1>::Get<UnderlyingType>(0), ms_to_process),
				static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

			std::for_each(std::execution::par_unseq, gln_sv.begin(), gln_sv.end(), [&](Sv litera_number) {
				auto intermediate_frequency = -(central_frequency - (1602e6 + static_cast<std::int32_t>(litera_number) * 0.5625e6));
				const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
				auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
					static_cast<std::size_t>(signal_sampling_rate));

				ProcessBpsk(downsampled_signal, code, litera_number, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
		}

		void ProcessGalileo(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::Galileo_E1b);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::Galileo_E1b);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::Galileo_E1b);

			auto intermediate_frequency = -(central_frequency - 1575.42e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));

			std::for_each(std::execution::par_unseq, galileo_sv.begin(), galileo_sv.end(), [&](auto sv) {
				auto samples_per_ms = static_cast<std::size_t>(new_sampling_rate / 1e3);
				const auto ref_code = UpsamplerType::Transform(PrnGenerator<Signal::Galileo_E1b>::Get<UnderlyingType>(sv.id),
					ms_to_process * samples_per_ms);
	
				std::vector<AcquisitionResult<UnderlyingType>> temporary_dst;
				std::array sign_permutations{
					std::array<int, 4>{	1,	1,	1,	1	},
					std::array<int, 4>{	1,	1,	1,	-1	},
					std::array<int, 4>{	1,	1,	-1,	-1	},
					std::array<int, 4>{	1,	-1,	-1,	-1	}
				};

				for (auto& sign_permutation : sign_permutations) {
					auto code = ref_code;
					for (std::size_t i = 0; i < sign_permutation.size(); ++i) {
						std::transform(code.begin() + i * samples_per_ms, code.begin() + (i + 1) * samples_per_ms, code.begin() + i * samples_per_ms,
							[cur_mul = sign_permutation[i]](auto& val) {return val * cur_mul; });
					}
					ProcessBpsk<false>(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, temporary_dst);
				}
				auto it = std::max_element(temporary_dst.begin(), temporary_dst.end());

				if (it->GetSnr() > peak_threshold) {
					auto lock = std::unique_lock(m);
					dst.push_back(std::move(*it));
				}
			});
		}

		void ProcessBeiDou(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::BeiDou_B1I);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::BeiDou_B1I);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::BeiDou_B1I);

			auto intermediate_frequency = -(central_frequency - 1561.098e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));

			std::for_each(std::execution::par_unseq, beidou_sv.begin(), beidou_sv.end(), [&](auto sv) {
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::BeiDou_B1I>::Get<UnderlyingType>(sv.id), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
		}

		void ProcessNavIC(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::NavIC_L5);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::NavIC_L5);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::NavIC_L5);

			auto intermediate_frequency = -(central_frequency - 1176.45e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));

			std::for_each(std::execution::par_unseq, navic_sv.begin(), navic_sv.end(), [&](auto sv) {
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::NavIC_L5>::Get<UnderlyingType>(sv.id), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
		}

		void ProcessSbas(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::Sbas_L5Q);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::Sbas_L5Q);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::Sbas_L5Q);

			auto intermediate_frequency = -(central_frequency - 1176.45e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate, acquisition_sampling_rate_L5);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));

			auto sbas_doppler_step = 10.0;
			std::swap(sbas_doppler_step, doppler_step);
			std::for_each(std::execution::par_unseq, sbas_sv.begin(), sbas_sv.end(), [&](auto sv) {
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::Sbas_L5Q>::Get<UnderlyingType>(sv.id), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk<true, false>(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
			});
			std::swap(sbas_doppler_step, doppler_step);
		}

		void ProcessQzss(const SignalEpoch<UnderlyingType>& epoch, std::vector<AcquisitionResult<UnderlyingType>>& dst) {
			const auto& signal = epoch.GetSubband(Signal::QzssCoarseAcquisition_L1);
			auto signal_sampling_rate = digital_frontend.GetSamplingRate(Signal::QzssCoarseAcquisition_L1);
			auto central_frequency = digital_frontend.GetCentralFrequency(Signal::QzssCoarseAcquisition_L1);

			auto intermediate_frequency = -(central_frequency - 1575.42e6);

			const auto translated_signal = MixerType::Translate(signal, signal_sampling_rate, -intermediate_frequency);
			auto new_sampling_rate = AdjustSamplingRate(signal_sampling_rate);
			auto downsampled_signal = Resampler<IppResampler>::Transform(translated_signal, static_cast<std::size_t>(new_sampling_rate),
				static_cast<std::size_t>(signal_sampling_rate));
			
			std::for_each(std::execution::par_unseq, qzss_sv.begin(), qzss_sv.end(), [&](auto sv) {
				const auto code = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<Signal::QzssCoarseAcquisition_L1>::Get<UnderlyingType>(sv.id), ms_to_process),
					static_cast<std::size_t>(ms_to_process * new_sampling_rate / 1e3));

				ProcessBpsk(downsampled_signal, code, sv, signal_sampling_rate, new_sampling_rate, intermediate_frequency, dst);
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
			auto& epoch_data = digital_frontend.GetSeveralEpochs(ms_offset, ms_to_process);

			if (plot_results) {
				if (digital_frontend.HasSignal(Signal::GpsCoarseAcquisition_L1))
					ugsdr::Add(L"GPS acquisition input signal", epoch_data.GetSubband(Signal::GpsCoarseAcquisition_L1), digital_frontend.GetSamplingRate(Signal::GpsCoarseAcquisition_L1));
				if (digital_frontend.HasSignal(Signal::GlonassCivilFdma_L1))
					ugsdr::Add(L"Glonass acquisition input signal", epoch_data.GetSubband(Signal::GlonassCivilFdma_L1), digital_frontend.GetSamplingRate(Signal::GlonassCivilFdma_L1));
				if (digital_frontend.HasSignal(Signal::Galileo_E1b))
					ugsdr::Add(L"Galileo acquisition input signal", epoch_data.GetSubband(Signal::Galileo_E1b), digital_frontend.GetSamplingRate(Signal::Galileo_E1b));
				if (digital_frontend.HasSignal(Signal::BeiDou_B1I))
					ugsdr::Add(L"BeiDou acquisition input signal", epoch_data.GetSubband(Signal::BeiDou_B1I), digital_frontend.GetSamplingRate(Signal::BeiDou_B1I));
				if (digital_frontend.HasSignal(Signal::NavIC_L5))
					ugsdr::Add(L"NavIC acquisition input signal", epoch_data.GetSubband(Signal::NavIC_L5), digital_frontend.GetSamplingRate(Signal::NavIC_L5));
				if (digital_frontend.HasSignal(Signal::Sbas_L5Q))
					ugsdr::Add(L"SBAS acquisition input signal", epoch_data.GetSubband(Signal::Sbas_L5Q), digital_frontend.GetSamplingRate(Signal::Sbas_L5Q));
				if (digital_frontend.HasSignal(Signal::QzssCoarseAcquisition_L1))
					ugsdr::Add(L"QZSS acquisition input signal", epoch_data.GetSubband(Signal::QzssCoarseAcquisition_L1), digital_frontend.GetSamplingRate(Signal::QzssCoarseAcquisition_L1));
			}
				
			if (digital_frontend.HasSignal(Signal::GpsCoarseAcquisition_L1))
				ProcessGps(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::GlonassCivilFdma_L1))
				ProcessGlonass(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::Galileo_E1b))
				ProcessGalileo(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::BeiDou_B1I))
				ProcessBeiDou(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::NavIC_L5))
				ProcessNavIC(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::Sbas_L5Q))
				ProcessSbas(epoch_data, dst);
			if (digital_frontend.HasSignal(Signal::QzssCoarseAcquisition_L1))
				ProcessQzss(epoch_data, dst);
		
			std::sort(dst.begin(), dst.end(), [](auto& lhs, auto& rhs) {
				return lhs.sv_number < rhs.sv_number;
			});

			if (plot_results)
				for (auto& acquisition_result : dst)
					ugsdr::Add(L"Satellite " + static_cast<std::wstring>(acquisition_result.sv_number), acquisition_result.output_peak);
		
			return dst;
		}
	};

	using FastSearchEngine = FastSearchEngineBase<float>;
}
