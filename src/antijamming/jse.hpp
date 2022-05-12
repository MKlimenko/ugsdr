#pragma once

#include "jamming_detection.hpp"
#include "../common.hpp"
#include "../math/abs.hpp"
#include "../math/dft.hpp"
#include "../math/max_index.hpp"
#include "../math/mean_stddev.hpp"

#ifdef HAS_IPP
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
	class JammingSuppressionEngine {
	private:
#ifdef HAS_IPP
		using AbsType = IppAbs;
		using DftType = IppDft;
		using MaxIndexType = IppMaxIndex;
		using MeanStdDevType = IppMeanStdDev;
#else
		using AbsType = SequentialAbs;
		using DftType = SequentialDft;
		using MaxIndexType = SequentialMaxIndex;
		using MeanStdDevType = SequentialMeanStdDev
#endif

		const double j_s_threshold = 20.0;
		double fs = 0.0;
		std::size_t max_inreference_cnt = 8;
		std::size_t filter_size = 128;
		std::size_t null_window_size = 3;
		std::size_t null_filter_size = 3;
		JammingDetector<std::complex<underlying_t<T>>> detector;
		
		template <typename Ty>
		void NullRegion(std::size_t index, std::size_t null_size, std::vector<Ty>& data) {
			for (std::ptrdiff_t i = index - null_size; i <= index + null_size; ++i) {
				if (i < 0 || i >= data.size())
					continue;
				data[i] = 0;
			}
		}

		void FilterInFreqencyDomain(std::vector<T>& src_dst) {
			DftType::Transform(src_dst);

			for (auto& el : detector.GetDetectionResults()) {
				auto idx = static_cast<std::size_t>(el.frequency / fs * src_dst.size());
				NullRegion(idx, null_window_size, src_dst);
			}
			DftType::Transform(src_dst, true);
		}

		void FilterWideband(std::vector<T>& src_dst) {
			auto& stft_data = detector.GetStft();
			const auto& intermediate_detection = detector.GetIntermediateResults();

			for (std::size_t i = 0; i < stft_data.size(); ++i) {
				auto& current_subvector = stft_data[i];
				const auto& current_detection = intermediate_detection[i];
				
				if (current_detection.empty())
					continue;
				if (i == 0)
					ugsdr::Add(L"Pre", current_subvector);
				NullRegion(current_detection[0].frequency_bin, null_filter_size, current_subvector);
				if (i == 0)
					ugsdr::Add(L"Post", current_subvector);
			}
			// Inverse stft
			//StftType::Transform(src_dst);
		}

	public:
		JammingSuppressionEngine(double sampling_rate) : fs(sampling_rate),
			filter_size(static_cast<std::size_t>(std::ceil(128.0 / 2.048e6 * fs))),
			null_window_size(static_cast<std::size_t>(std::ceil(3.0 / 2.048e6 * fs))),
			detector(sampling_rate) {}

		void Process(std::vector<T>& src_dst) {
			const auto& detection_results = detector.Process(src_dst);
			if (detection_results.empty())
				return;
			if (std::none_of(detection_results.begin(), detection_results.end(), [](const auto& val) { return val.is_wideband(); }))
				// narrowband
				FilterInFreqencyDomain(src_dst);
			else
				FilterWideband(src_dst);
		}

		const auto& GetDetectionResults() const {
			return detector.GetDetectionResults();
		}
	};
}
