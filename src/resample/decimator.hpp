#pragma once

#include "../helpers/is_complex.hpp"

#include <cmath>
#include <vector>

namespace ugsdr {
	template <typename DecimatorImpl>
	class Decimator {
	protected:
	public:
		template <typename T>
		static void Transform(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			DecimatorImpl::Process(src_dst, decimation_ratio);
		}

		template <typename T>
		static auto Transform(const std::vector<T>& src_dst, std::size_t decimation_ratio) {
			auto dst = src_dst;
			DecimatorImpl::Process(dst, decimation_ratio);
			return dst;
		}
	};

	class SequentialDecimator : public Decimator<SequentialDecimator> {
	protected:
		friend class Decimator<SequentialDecimator>;

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			for (std::size_t i = 0, j = 0; i < src_dst.size(); i += decimation_ratio)
				src_dst[j++] = src_dst[i];
			src_dst.resize(src_dst.size() / decimation_ratio);
		}

	public:
	};
	
	class Accumulator : public Decimator<Accumulator> {
	protected:
		friend class Decimator<Accumulator>;

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			if (decimation_ratio == 1)
				return;
			auto [rem, quot] = std::div(src_dst.size(), static_cast<std::int64_t>(decimation_ratio));

			if (quot)
				src_dst.resize((rem + 1) * decimation_ratio);
			
			for (std::size_t i = 0, k = 0; i < src_dst.size(); i += decimation_ratio) {
				auto tmp = T{};
				for (std::size_t j = 0; j < decimation_ratio; ++j)
					tmp += src_dst[i + j];
				src_dst[k++] = tmp / static_cast<underlying_t<T>>(decimation_ratio);
			}
			src_dst.resize(src_dst.size() / decimation_ratio);
		}

	public:
	};
}
