#pragma once

#include "ipp.h"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	template <typename T>
	struct IppTypeToComplex {
		using ComplexTypeMap = mk::TypeMap<
			mk::TypePair<Ipp16s, Ipp16sc>,
			mk::TypePair<Ipp32s, Ipp32sc>,
			mk::TypePair<Ipp32f, Ipp32fc>,
			mk::TypePair<Ipp64f, Ipp64fc>
		>;

		using Type = ComplexTypeMap::GetTypeByType<T>;
	};

	template <typename T>
	struct IppComplexToType {
		using ComplexTypeMap = mk::TypeMap<
			mk::TypePair<Ipp16sc, Ipp16s>,
			mk::TypePair<Ipp32sc, Ipp32s>,
			mk::TypePair<Ipp32fc, Ipp32f>,
			mk::TypePair<Ipp64fc, Ipp64f>
		>;

		using Type = ComplexTypeMap::GetTypeByType<T>;
	};
}
