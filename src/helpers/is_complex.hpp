#pragma once

#include <complex>

namespace ugsdr {
	template <typename>
	static constexpr bool is_complex_v = false;
	template <typename T>
	static constexpr bool is_complex_v<std::complex<T>> = true;

	template <typename T>
	struct underlying {
		using type = T;
	};

	template <typename T>
	struct underlying<std::complex<T>> {
		using type = T;
	};

	template <typename T>
	using underlying_t = typename underlying<T>::type;
}
