#pragma once

#ifdef HAS_ARRAYFIRE

#include "conj.hpp"
#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfConj : public ComplexConjugate<AfConj> {
	private:
	protected:
		friend class ComplexConjugate<AfConj>;

		static auto Process(const ArrayProxy& src_dst) {
			return ArrayProxy(af::conjg(src_dst));
		}

		template <typename T>
		static auto Process(const std::vector<T>& src) {
			return Process(ArrayProxy(src));
		}
	};
}

#endif
