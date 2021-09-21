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
		}

	public:
	};
}
