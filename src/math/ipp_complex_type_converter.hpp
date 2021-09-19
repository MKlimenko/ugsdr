#pragma once

#include "ipp.h"

namespace ugsdr {
	template <typename T>
	struct IppComplexTypeConverter {
		// pseudo type map
		using Type = std::conditional_t <std::is_same_v<std::int16_t, T>, Ipp16sc,
			std::conditional_t<std::is_same_v<float, T>, Ipp32fc,
			std::conditional_t<std::is_same_v<double, T>, Ipp64fc, void>>>;

		static_assert(!std::is_same_v<Type, void>, "This type is not supported (yet)");
	};
}
