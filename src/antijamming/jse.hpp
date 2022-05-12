#pragma once

#include "jamming_detection.hpp"
#include "../common.hpp"
#include "../math/abs.hpp"
#include "../math/dft.hpp"
#include "../math/max_index.hpp"
#include "../math/mean_stddev.hpp"
#include "../math/stft.hpp"

#ifdef HAS_IPP
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
	class JammingSuppressionEngine {
	private:
#ifdef HAS_IPP
		using AbsType = IppAbs;
		using DftType = IppDft;
		using MaxIndexType = IppMaxIndex;
		using MeanStdDevType = IppMeanStdDev;
		using StftType = IppStft;
#else
		using AbsType = SequentialAbs;
		using DftType = SequentialDft;
		using MaxIndexType = SequentialMaxIndex;
		using MeanStdDevType = SequentialMeanStdDev;
		using StftType = SequentialStft;
#endif

		const double j_s_absolute_threshold = 20.0;
		double fs = 0.0;
		std::size_t max_inreference_cnt = 8;
		std::size_t filter_size = 128;
		std::size_t null_window_size = 128;
		JammingDetector<std::complex<underlying_t<T>>> detector;
		
		template <typename Ty>
		void NullRegion(std::size_t index, std::size_t null_size, std::vector<Ty>& data) {
			auto start = static_cast<std::ptrdiff_t>(index) - static_cast<std::ptrdiff_t>(null_size);
			auto finish = static_cast<std::ptrdiff_t>(index) + static_cast<std::ptrdiff_t>(null_size);
			start += data.size();
			if (finish < start)
				finish += data.size();
			for (std::ptrdiff_t i = start; i <= finish; ++i)
				data[i % data.size()] = 0;
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
				for (auto& el : current_subvector)
					if (std::abs(el) >= j_s_absolute_threshold)
						el = 0;
			}
			auto initial_size = src_dst.size();
			src_dst = StftType::Transform(stft_data);
			
			// todo: fix inverse stft
			if (src_dst.size() != initial_size)
				src_dst.resize(initial_size);
		}

	public:
		JammingSuppressionEngine(double sampling_rate) : fs(sampling_rate),
			filter_size(static_cast<std::size_t>(std::ceil(128.0 / 2.048e6 * fs))),
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
