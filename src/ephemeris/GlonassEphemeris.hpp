#pragma once

#include "../common.hpp"

#include <boost/dynamic_bitset.hpp>

#include <numbers>
#include <span>

namespace ugsdr {
	struct GlonassEphemeris final {
	private:
		// funcs

	public:
		// data

		GlonassEphemeris() = default;

		template <typename T>
		GlonassEphemeris(std::span<T> nav_bits) {

		}
	};
}
