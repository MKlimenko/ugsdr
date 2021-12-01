#pragma once

#if __has_include(<arrayfire.h>)

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
			auto spectrum = Process(const_cast<const std::vector<std::complex<UnderlyingType>>&>(src_dst), is_inverse);
					
			auto spectrum_cpu_optional = spectrum.CopyFromGpu(src_dst);

			if (spectrum_cpu_optional.has_value())
				src_dst = std::move(spectrum_cpu_optional.value());
		}

		static void Process(ArrayProxy& src, bool is_inverse = false) {
			if (is_inverse)
				af::ifftInPlace(src);
			else
				af::fftInPlace(src);
		}

		template <typename T>
		static auto Process(const std::vector<T>& src, bool is_inverse = false) {
			auto af_src = ArrayProxy(src);
			ArrayProxy dft_transform;
			if (is_inverse)
				dft_transform = ArrayProxy(af::idft(af_src));
			else
				dft_transform = ArrayProxy(af::dft(af_src));

			return dft_transform;
		}
	};
}

#endif
