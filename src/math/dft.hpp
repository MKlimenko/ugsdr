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
			DftImpl::Process(src_dst, is_inverse);
		}

		template <typename T>
		static void Transform(T& src_dst, bool is_inverse = false) {
			DftImpl::Process(src_dst, is_inverse);
		}
		
		template <typename UnderlyingType>
		static auto Transform(const std::vector<UnderlyingType>& src, bool is_inverse = false) {
			return DftImpl::Process(src, is_inverse);
		}
	};
}
