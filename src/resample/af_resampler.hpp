#pragma once

#include "resampler.hpp"

#ifdef HAS_ARRAYFIRE

#include "af_decimator.hpp"
#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfResampler: public Resampler<AfResampler> {
		static void Upsample(ArrayProxy& src_dst, std::size_t samples) {
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

	protected:
		friend class Resampler<AfResampler>;

		static void Process(ArrayProxy& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto lcm = std::lcm(new_sampling_rate, old_sampling_rate);
			auto new_samples = lcm * src_dst.size() / old_sampling_rate;
			Upsample(src_dst, new_samples);
			AfAccumulator::Transform(src_dst, lcm / new_sampling_rate);
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
