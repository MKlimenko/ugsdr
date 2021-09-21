#pragma once

#include <cmath>
#include <vector>

namespace ugsdr {
	template <typename DecimatorImpl>
	class Decimator {
	protected:
	public:
		template <typename UnderlyingType>
		static void Transform(std::vector<UnderlyingType>& src_dst, std::size_t decimation_ratio) {
			DecimatorImpl::Process(src_dst, decimation_ratio);
		}

		template <typename UnderlyingType>
		static auto Transform(const std::vector<UnderlyingType>& src_dst, std::size_t decimation_ratio) {
			auto dst = src_dst;
			DecimatorImpl::Process(dst, decimation_ratio);
			return dst;
		}
	};

	class SequentialDecimator : public Decimator<SequentialDecimator> {
	protected:
		friend class Decimator<SequentialDecimator>;

		template <typename UnderlyingType>
		static void Process(std::vector<UnderlyingType>& src_dst, std::size_t decimation_ratio) {
			for (std::size_t i = 0, j = 0; i < src_dst.size(); i += decimation_ratio)
				src_dst[j++] = src_dst[i];
			src_dst.resize(src_dst.size() / decimation_ratio);
		}

	public:
	};
	
	class Accumulator : public Decimator<Accumulator> {
	protected:
		friend class Decimator<Accumulator>;

		template <typename UnderlyingType>
		static void Process(std::vector<UnderlyingType>& src_dst, std::size_t decimation_ratio) {
			for (std::size_t i = 0, k = 0; i < src_dst.size(); i += decimation_ratio) {
				auto tmp = UnderlyingType{};
				for (std::size_t j = 0; j < decimation_ratio; ++j)
					tmp += src_dst[i + j];
				src_dst[k++] = static_cast<UnderlyingType>(tmp / static_cast<double>(decimation_ratio));
			}
			src_dst.resize(src_dst.size() / decimation_ratio);
			src_dst.shrink_to_fit();
		}

	public:
	};
}
