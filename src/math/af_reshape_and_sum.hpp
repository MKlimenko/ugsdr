#pragma once

#include "reshape_and_sum.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfReshapeAndSum : public ReshapeAndSum<AfReshapeAndSum> {
	protected:
		friend class ReshapeAndSum<AfReshapeAndSum>;
		
		static auto Process(const ArrayProxy& src_dst, std::size_t block_size) {
			auto matrix = af::moddims(src_dst, block_size, src_dst.size() / block_size);
			auto dst = af::sum(matrix, 1, 0.0);

			return ArrayProxy(dst);
		}
	};
}

#endif
