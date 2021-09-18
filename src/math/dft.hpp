#pragma once

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename DftImpl>
	class DiscreteFourierTransform {
	protected:
	public:
		// TODO: Add real-to-complex and real-to-real transforms (when required)
		template <typename UnderlyingType>
		static void Transform(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			DftImpl::Process(src_dst);
		}

		template <typename UnderlyingType>
		static auto Transform(const std::vector<std::complex<UnderlyingType>>& src, bool is_inverse = false) {
			auto dst = src;
			DftImpl::Process(dst);
			return dst;
		}
	};
}
