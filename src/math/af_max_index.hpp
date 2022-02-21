#pragma once

#include "max_index.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfMaxIndex : public MaxIndex<AfMaxIndex> {
	protected:
		friend class MaxIndex<AfMaxIndex>;
		
		static auto Process(const ArrayProxy& src_dst) {
			double max_value = 0.0;
			unsigned max_index = 0;
			af::max(&max_value, &max_index, src_dst);
			auto result = Result<double, unsigned>{ max_value, max_index };
			return result;
		}
	};
}

#endif
