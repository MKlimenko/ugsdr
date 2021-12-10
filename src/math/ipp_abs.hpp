#pragma once

#include "abs.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	class IppAbs : public Abs<IppAbs> {
	private:
		static auto GetAbsWrapper() {
			static auto abs_wrapper = plusifier::FunctionWrapper(
				ippsAbs_32fc_A11, ippsAbs_64fc_A26);

			return abs_wrapper;
		}

	protected:
		friend class Abs<IppAbs>;

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src_dst) {
			std::vector<UnderlyingType> dst(src_dst.size());
			auto abs_wrapper = GetAbsWrapper();
			
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			abs_wrapper(reinterpret_cast<const IppType*>(src_dst.data()), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(src_dst.size()));
			return dst;
		}
	};
}

#endif
