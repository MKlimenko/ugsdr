#pragma once

#include <cstdint>
#include <cstring>
#include <numbers>

namespace ugsdr {
	template <std::size_t nco_bits = 32>
	struct NumericallyControlledOscillator final {
		static_assert(nco_bits <= 64, "Requested size of NCO is not supported (requires long math)");
		constexpr static inline std::uint64_t mask = (1ull << nco_bits) - 1;
		
		std::uint64_t holder = 0;
		std::uint64_t adder = 0;

		NumericallyControlledOscillator() = default;
		NumericallyControlledOscillator(double sampling_freq, double frequency, double phase) {
			adder = static_cast<std::uint64_t>((1ull << nco_bits) * frequency / sampling_freq);
			holder = static_cast<std::uint64_t>((1ull << nco_bits) * phase / (2 * std::numbers::pi));
		}
		
	};
}
