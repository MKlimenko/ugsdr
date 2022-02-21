#pragma once

#include "resampler.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfResampler: public Resampler<AfResampler> {
	protected:
		friend class Resampler<AfResampler>;

		static void Process(ArrayProxy& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto iota_type = af::dtype::f32;

			switch (static_cast<af::array>(src_dst).type()) {
			case af::dtype::f64:
			case af::dtype::c64:
				iota_type = af::dtype::f64;
				break;
			default:
				break;
			}

			double ratio = static_cast<double>(new_sampling_rate) / static_cast<double>(old_sampling_rate);
			auto points = ArrayProxy(
				af::iota(src_dst.size() * ratio, af::dim4(1), iota_type) / ratio
			);

			src_dst = af::approx1(src_dst, points);
		}

		template <Container T>
		static void Process(T& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto gpu_dst = ArrayProxy(const_cast<const T&>(src_dst));
			Process(gpu_dst, new_sampling_rate, old_sampling_rate);

			auto dst_optional = gpu_dst.CopyFromGpu(src_dst);

			if (dst_optional.has_value())
				src_dst = std::move(dst_optional.value());
		}
	};
}
#endif
