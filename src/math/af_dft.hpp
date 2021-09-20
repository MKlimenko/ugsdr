#pragma once

#include "dft.hpp"
#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfDft : public DiscreteFourierTransform<AfDft> {
	private:
	protected:
		friend class DiscreteFourierTransform<AfDft>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			auto af_src = ArrayProxy(const_cast<const std::vector<std::complex<UnderlyingType>>&>(src_dst));
			auto spectrum = ArrayProxy(af::dft(af_src));

			auto spectrum_cpu_optional = spectrum.CopyFromGpu(src_dst);

			if (spectrum_cpu_optional.has_value())
				src_dst = std::move(spectrum_cpu_optional.value());
		}

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src, bool is_inverse = false) {
			auto dst = src;
			Process(dst, is_inverse);
			return src;
		}
		template <typename UnderlyingType>
		static auto Process(const std::vector<UnderlyingType>& src, bool is_inverse = false) {
			auto dst = std::vector<std::complex<UnderlyingType>>(src.begin(), src.end());
			Process(dst, is_inverse);
			return dst;
		}
	};
}
