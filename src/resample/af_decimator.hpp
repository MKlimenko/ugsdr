#pragma once

#include "decimator.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfDecimator : public Decimator<AfDecimator> {
	protected:
		friend class Decimator<AfDecimator>;

		static void Process(ArrayProxy& src_dst, std::size_t decimation_ratio) {
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
				af::iota(src_dst.size() / decimation_ratio, af::dim4(1), iota_type) * decimation_ratio
			);

			src_dst = af::approx1(src_dst, points);
		}

		template <Container T>
		static void Process(T& src_dst, std::size_t decimation_ratio) {
			auto gpu_dst = ArrayProxy(const_cast<const T&>(src_dst));
			Process(gpu_dst, decimation_ratio);

			auto dst_optional = gpu_dst.CopyFromGpu(src_dst);

			if (dst_optional.has_value())
				src_dst = std::move(dst_optional.value());
		}
	};


	class AfAccumulator : public Decimator<AfAccumulator> {
	protected:
		friend class Decimator<AfAccumulator>;

		static void Process(ArrayProxy& src_dst, std::size_t decimation_ratio) {
			auto iota_type = af::dtype::f32;

			switch (static_cast<af::array>(src_dst).type()) {
			case af::dtype::f64:
			case af::dtype::c64:
				iota_type = af::dtype::f64;
				break;
			default:
				break;
			}

			auto matrix = af::moddims(src_dst, decimation_ratio, src_dst.size() / decimation_ratio);
			src_dst = ArrayProxy((af::sum(matrix, 0, 0.0) / decimation_ratio).T());
		}

		template <Container T>
		static void Process(T& src_dst, std::size_t decimation_ratio) {
			auto gpu_dst = ArrayProxy(const_cast<const T&>(src_dst));
			Process(gpu_dst, decimation_ratio);

			auto dst_optional = gpu_dst.CopyFromGpu(src_dst);

			if (dst_optional.has_value())
				src_dst = std::move(dst_optional.value());
		}
	};
}
#endif
