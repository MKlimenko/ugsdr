#pragma once

#ifdef HAS_ARRAYFIRE

#include "abs.hpp"
#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfAbs : public Abs<AfAbs> {
	private:
	protected:
		friend class Abs<AfAbs>;

		static auto Process(const ArrayProxy& src_dst) {
			return ArrayProxy(af::abs(src_dst));
		}

		template <typename T>
		static auto Process(const std::vector<T>& src) {
			return Process(ArrayProxy(src));
		}
	};
}

#endif
