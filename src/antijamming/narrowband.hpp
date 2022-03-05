#pragma once

#include "../common.hpp"
#include "../digital_filter/fir.hpp"
#include "../math/abs.hpp"
#include "../math/dft.hpp"
#include "../math/max_index.hpp"
#include "../math/mean_stddev.hpp"

#ifdef HAS_IPP
#include "../digital_filter/ipp_fir.hpp"
#include "../math/ipp_abs.hpp"
#include "../math/ipp_dft.hpp"
#include "../math/ipp_max_index.hpp"
#include "../math/ipp_mean_stddev.hpp"
#endif

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace ugsdr {
	template <typename T>
	class NarrowbandSuppressor {
	public:
		struct DetectionResult {
			std::size_t frequency_bin = 0.0;
			double frequency = 0.0;
			double jammer_to_noise = 0.0;

			DetectionResult() = default;
			DetectionResult(std::size_t bin, double freq, double j_n_ratio) : frequency_bin(bin), frequency(freq), jammer_to_noise(j_n_ratio) {}
		};

	private:
#ifdef HAS_IPP
		using AbsType = IppAbs;
		using DftType = IppDft;
		using FirType = SequentialFir<T, std::vector<T>, std::vector<T>>;
		using MaxIndexType = IppMaxIndex;
		using MeanStdDevType = IppMeanStdDev;
#else
		using AbsType = SequentialAbs;
		using DftType = SequentialDft;
		using FirType = SequentialFir<T, std::vector<T>, std::vector<T>>;
		using MaxIndexType = SequentialMaxIndex;
		using MeanStdDevType = SequentialMeanStdDev
#endif

		const double j_s_threshold = 20.0;
		double fs = 0.0;
		std::size_t max_inreference_cnt = 8;
		std::size_t filter_size = 128;
		std::size_t null_window_size = 3;
		std::size_t null_filter_size = 3;
		FirType fir;
		std::vector<underlying_t<T>> detection_window;
		std::vector<underlying_t<T>> ir_window;
		std::vector<T> magnitude;
		std::vector<DetectionResult> detection_results;

		auto GetWindowedSignal(const std::vector<T>& src_dst) {
			std::vector<T> windowed_signal;
			windowed_signal.resize(detection_window.size());
			std::transform(std::execution::par_unseq, detection_window.begin(), detection_window.end(), src_dst.begin(), windowed_signal.begin(), std::multiplies<T>{});

			return windowed_signal;
		}

		static auto GenerateDetectionWindow(std::size_t window_size) {
			std::vector<underlying_t<T>> detection_window(window_size);
			constexpr std::array<double, 4> a = { 0.3635819, 0.4891775, 0.1365995, 0.0106411 };

			for (std::size_t i = 0; i < window_size; ++i)
				for (std::size_t j = 0; j < a.size(); ++j)
					detection_window[i] += a[j] * std::pow(-1, j) * cos(2.0 * j * std::numbers::pi * i / (window_size - 1));

			return detection_window;
		}

#ifdef HAS_IPP
		[[nodiscard]]
		static auto GetKaiserWrapper() {
			static auto kaiser_wrapper = plusifier::FunctionWrapper(
				ippsWinKaiser_32f_I,
				ippsWinKaiser_64f_I
			);

			return kaiser_wrapper;
		}
#endif

		static auto GenerateIrWindow(std::size_t window_size) {
			std::vector<underlying_t<T>> ir_window(window_size, 1);
#ifdef HAS_IPP
			auto window = GetKaiserWrapper();
			window(ir_window.data(), static_cast<int>(window_size), 5.5);
#else
			// suboptimal, implement standalone Kaiser window
			ir_window = GenerateDetectionWindow(window_size);
#endif
			return ir_window;
		}

		template <typename Ty>
		void NullRegion(std::size_t index, std::size_t null_size, std::vector<Ty>& magnitude) {
			for (std::ptrdiff_t i = index - null_size; i <= index + null_size; ++i) {
				if (i < 0 || i >= magnitude.size())
					continue;
				magnitude[i] = 0;
			}
		}

		bool DetectInterference(const std::vector<T>& src_dst) {
			std::vector<DetectionResult> jammer_candidates;
			jammer_candidates.reserve(max_inreference_cnt);

			const auto windowed_signal = GetWindowedSignal(src_dst);
			auto magnitude = AbsType::Transform(DftType::Transform(windowed_signal));
			for (std::size_t i = 0; i < this->max_inreference_cnt; ++i) {
				auto [value, index] = MaxIndexType::Transform(magnitude);
				auto fir_index = static_cast<std::size_t>(std::ceil(index / (fs / 1e3) * filter_size));
				jammer_candidates.emplace_back(fir_index, index * fs / magnitude.size(), value);
				NullRegion(index, null_window_size, magnitude);
			}
			auto [mean, stddev] = MeanStdDevType::Calculate(magnitude);
			for (auto& el : jammer_candidates)
				el.jammer_to_noise /= stddev;

			jammer_candidates.erase(std::remove_if(jammer_candidates.begin(), jammer_candidates.end(), [this](auto& val) {
				return val.jammer_to_noise < j_s_threshold;
			}), jammer_candidates.end());

			bool is_same = std::equal(detection_results.begin(), detection_results.end(), jammer_candidates.begin(), jammer_candidates.end(), 
				[](const auto& lhs, const auto& rhs) {
					return lhs.frequency_bin == rhs.frequency_bin;
				});
			detection_results = jammer_candidates;
			return is_same;
		}

		void UpdateImpulseResponse() {
			std::vector<std::complex<underlying_t<T>>> frequency_response(filter_size, 1);
			for (auto& el : detection_results)
				NullRegion(el.frequency_bin, null_filter_size, frequency_response);
			underlying_t<T> sign = 1;
			for(std::size_t i = 0; i < frequency_response.size(); ++i) {
				frequency_response[i] *= sign;
				sign = -sign;
			}
			auto ir = DftType::Transform(const_cast<const std::vector<std::complex<underlying_t<T>>>&>(frequency_response), true);
			std::transform(ir.begin(), ir.end(), ir_window.begin(), ir.begin(), std::multiplies<T>{});
			fir.UpdateWeights(ir);
		}

		void FilterInFreqencyDomain(std::vector<T>& src_dst) {
			DftType::Transform(src_dst);

			for(auto&el:detection_results) {
				auto idx = static_cast<std::size_t>(el.frequency / fs * src_dst.size());
				NullRegion(idx, null_window_size, src_dst);
			}
			DftType::Transform(src_dst, true);
		}

	public:
		NarrowbandSuppressor(double sampling_rate) : fs(sampling_rate),
			filter_size(static_cast<std::size_t>(std::ceil(128.0 / 2.048e6 * fs))),
			null_window_size(static_cast<std::size_t>(std::ceil(3.0 / 2.048e6 * fs))), fir(this->filter_size),
			detection_window(GenerateDetectionWindow(static_cast<std::size_t>(fs / 1e3))),
			ir_window(GenerateIrWindow(filter_size)),
			magnitude(this->filter_size, static_cast<underlying_t<T>>(1)) {

			std::vector<T> ir_placeholder(filter_size, 0);
			ir_placeholder[0] = 1;
			fir.UpdateWeights(ir_placeholder);
		}

		void Process(std::vector<T>& src_dst) {
			bool is_same = DetectInterference(src_dst);
			if (!detection_results.empty())
				FilterInFreqencyDomain(src_dst);
			//bool is_same = DetectInterference(src_dst);
			//if (!is_same)
				//UpdateImpulseResponse();
			//fir.Filter(src_dst);
		}

		const auto& GetDetectionResults() const {
			return detection_results;
		}

		const auto& GetImpulseResponse() const {
			return fir.GetWeigts();
		}
	};
}
