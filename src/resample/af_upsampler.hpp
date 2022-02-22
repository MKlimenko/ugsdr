#pragma once

#include "upsampler.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfUpsampler: public Upsampler<AfUpsampler> {
	protected:
		friend class Upsampler<AfUpsampler>;

		static void Process(ArrayProxy& src_dst, std::size_t samples) {
			auto iota_type = af::dtype::f32;

			switch (static_cast<af::array>(src_dst).type()) {
			case af::dtype::f64:
			case af::dtype::c64:
				iota_type = af::dtype::f64;
				break;
			default:
				break;
			}

			auto points = ArrayProxy(
				af::iota(samples, af::dim4(1), iota_type) * static_cast<double>(src_dst.size()) / samples
			);
			src_dst = ArrayProxy(
				af::approx1(src_dst, points, af::interpType::AF_INTERP_NEAREST)
			);
		}

		template <Container T>
		static void Process(T& src_dst, std::size_t samples) {
			auto gpu_dst = ArrayProxy(const_cast<const T&>(src_dst));
			Process(gpu_dst, samples);

			auto dst_optional = gpu_dst.CopyFromGpu(src_dst);

			if (dst_optional.has_value())
				src_dst = std::move(dst_optional.value());
		}
	};
}
#endif
