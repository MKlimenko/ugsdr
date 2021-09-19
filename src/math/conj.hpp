#pragma once

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename ConjImpl>
	class ComplexConjugate {
	protected:
	public:
		template <typename UnderlyingType>
		static void Transform(std::vector<std::complex<UnderlyingType>>& src_dst) {
			ConjImpl::Process(src_dst);
		}

		template <typename UnderlyingType>
		static auto Transform(const std::vector<std::complex<UnderlyingType>>& src) {
			auto dst = src;
			ConjImpl::Process(dst);
			return dst;
		}
	};
}
