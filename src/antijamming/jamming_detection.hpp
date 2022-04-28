#pragma once

#include "../common.hpp"
#include "../digital_filter/fir.hpp"
#include "../math/abs.hpp"
#include "../math/dft.hpp"
#include "../math/max_index.hpp"
#include "../math/mean_stddev.hpp"
#include "../math/stft.hpp"

#ifdef HAS_IPP
#include "../digital_filter/ipp_customized_fir.hpp"
#include "../math/ipp_abs.hpp"
#include "../math/ipp_dft.hpp"
#include "../math/ipp_max_index.hpp"
#include "../math/ipp_mean_stddev.hpp"
#include "../math/ipp_stft.hpp"
#endif

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace ugsdr {
	template <typename T>
	class JammingDetector {
	public:
		struct DetectionResult {
			enum class InterferenceType : std::size_t {
				None = 0,
				Narrowband,
				Wideband
			};

			InterferenceType interference_type = InterferenceType::None;
			std::size_t frequency_bin = 0.0;
			double frequency = 0.0;
			double jammer_to_noise = 0.0;

			DetectionResult() = default;
			DetectionResult(InterferenceType type, std::size_t bin, double freq, double j_n_ratio) : interference_type(type),
				frequency_bin(bin), frequency(freq), jammer_to_noise(j_n_ratio) {}
		};

	private:
#ifdef HAS_IPP
		using AbsType = IppAbs;
		using DftType = IppDft;
		using FirType = IppCustomizedFir<T, std::vector<T>, std::vector<T>>;
		using MaxIndexType = IppMaxIndex;
		using MeanStdDevType = IppMeanStdDev;
		using StftType = IppStft;
#else
		using AbsType = SequentialAbs;
		using DftType = SequentialDft;
		using FirType = SequentialFir<T, std::vector<T>, std::vector<T>>;
		using MaxIndexType = SequentialMaxIndex;
		using MeanStdDevType = SequentialMeanStdDev
		using StftType = SequentialStft;
#endif
		struct InternalDetectionResult {
			std::size_t frequency_bin = 0.0;
			double frequency = 0.0;
			double jammer_to_noise = 0.0;

			InternalDetectionResult() = default;
			InternalDetectionResult(std::size_t bin, double freq, double j_n_ratio) : frequency_bin(bin), frequency(freq), jammer_to_noise(j_n_ratio) {}
		};

		const double j_s_threshold = 25.0;
		const double probability_threshold = 0.9;
		double fs = 0.0;
		std::size_t max_inreference_cnt = 8;
		std::size_t null_window_size = 3;
		std::vector<underlying_t<T>> detection_window;
		std::vector<DetectionResult> detection_results;

		auto GetWindowedSignal(const std::vector<T>& src_dst) {
			std::vector<T> windowed_signal;
			windowed_signal.resize(detection_window.size());
			std::transform(std::execution::par_unseq, detection_window.begin(), detection_window.end(), src_dst.begin(), windowed_signal.begin(), std::multiplies<T>{});

			return windowed_signal;
		}

		constexpr static auto GenerateDetectionWindow(std::size_t window_size) {
			std::vector<underlying_t<T>> detection_window(window_size);
			constexpr std::array<double, 4> a = { 0.3635819, 0.4891775, 0.1365995, 0.0106411 };

			for (std::size_t i = 0; i < window_size; ++i)
				for (std::size_t j = 0; j < a.size(); ++j)
					detection_window[i] += a[j] * std::pow(-1, j) * cos(2.0 * j * std::numbers::pi * i / (window_size - 1));

			return detection_window;
		}
		
		template <typename Ty>
		static void NullRegion(std::size_t index, std::size_t null_size, std::vector<Ty>& magnitude) {
			for (std::ptrdiff_t i = index - null_size; i <= index + null_size; ++i) {
				if (i < 0 || i >= magnitude.size())
					continue;
				magnitude[i] = 0;
			}
		}


		template <typename Ty>
		auto IntermediateDetection(const std::vector<std::vector<std::complex<Ty>>>& stft_results) {
			std::vector<std::vector<InternalDetectionResult>> intermediate(stft_results.size());

			std::for_each(std::execution::par_unseq, stft_results.begin(), stft_results.end(), [&stft_results, &intermediate, this](auto& current_subvector) {
				auto& jammer_candidates = intermediate[std::distance(stft_results.data(), &current_subvector)];
				auto magnitude = AbsType::Transform(current_subvector);
				for (std::size_t i = 0; i < max_inreference_cnt; ++i) {
					auto [value, index] = MaxIndexType::Transform(magnitude);
					jammer_candidates.emplace_back(index, index * fs / magnitude.size(), value);
					NullRegion(index, null_window_size, magnitude);
				}
				auto [mean, stddev] = MeanStdDevType::Calculate(magnitude);
				for (auto& el : jammer_candidates)
					el.jammer_to_noise /= stddev;

				jammer_candidates.erase(std::remove_if(jammer_candidates.begin(), jammer_candidates.end(), [this](auto& val) {
					return val.jammer_to_noise < j_s_threshold;
				}), jammer_candidates.end());
			});

			return intermediate;
		}

		auto AnalyzeStftData(const std::vector<std::vector<InternalDetectionResult>>& intermediate_results) const {
			std::vector<DetectionResult> dst;

			std::size_t mean_number_of_jammers = 0;
			for (auto& el : intermediate_results) 
				mean_number_of_jammers += el.size();
			mean_number_of_jammers /= intermediate_results.size();

			for(std::size_t i = 0; i < mean_number_of_jammers; ++i) {
				std::map<double, std::pair<double, InternalDetectionResult>> freq_occupation; // mapping frequency to count and internal_results
				double current_quantity = 0;
				for (auto& el : intermediate_results) {
					if (el.size() <= i)
						continue;
					// todo: refactor, not the best way to do it
					const auto& current_jammer_info = el[i];
					freq_occupation[current_jammer_info.frequency].first += 1;
					freq_occupation[current_jammer_info.frequency].second.frequency = el[i].frequency;
					freq_occupation[current_jammer_info.frequency].second.frequency_bin = el[i].frequency_bin;
					freq_occupation[current_jammer_info.frequency].second.jammer_to_noise += el[i].jammer_to_noise;
					++current_quantity;
				}
				for (auto& [freq, cur_info] : freq_occupation) {
					cur_info.first /= current_quantity;
					cur_info.second.jammer_to_noise /= current_quantity;
				}

				std::size_t cnt_wideband = 0;
				auto* wideband_ptr = &freq_occupation.begin()->second.second;
				for (auto& el : freq_occupation) {
					// narrowband
					auto probability = el.second.first;
					if (probability >= probability_threshold) {
						const auto& internal_result = el.second.second;
						dst.emplace_back(DetectionResult::InterferenceType::Narrowband, internal_result.frequency_bin,
							internal_result.frequency, internal_result.jammer_to_noise);
						break;
					}
					// wideband
					if (probability >= probability_threshold / static_cast<double>(freq_occupation.size())) {
						++cnt_wideband;
						wideband_ptr = &el.second.second;
					}
					
				}

				if (cnt_wideband >= static_cast<std::size_t>(freq_occupation.size() * probability_threshold))
					dst.emplace_back(DetectionResult::InterferenceType::Wideband, wideband_ptr->frequency_bin,
						wideband_ptr->frequency, wideband_ptr->jammer_to_noise);
			}

			return dst;
		}
		

	public:
		JammingDetector(double sampling_rate) : fs(sampling_rate),
			detection_window(GenerateDetectionWindow(static_cast<std::size_t>(fs / 1e3))) {}

		const auto& Process(const std::vector<T>& src_dst) {
			auto stft = StftType::Transform(src_dst);
			auto intermediate_results = IntermediateDetection(stft);
			detection_results = AnalyzeStftData(intermediate_results);
			// stft
			// get indexes
			//
			return detection_results;
		}

		const auto& GetDetectionResults() const {
			return detection_results;
		}
	};
}
