#pragma once

#include "codegen.hpp"

#include <array>

namespace ugsdr {
	class L5 {
	private:
		static constexpr std::size_t code_len = 10230;
		static constexpr std::size_t initial_value = 0xFFFF;
		static constexpr std::array<std::uint32_t, 2> lfsr_poly = { 0x1B00, 0x18ED };
		static constexpr std::size_t lfsr_output_pin = 12;

	protected:
		static auto NumberOfMilliseconds() {
			return 1;
		}

		static auto GetCodeLength() {
			return code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number, std::size_t(*SecondLfsrPhase)(std::size_t)) {
			std::array<Lfsr, 2> lfsr;
			for (std::size_t i = 0; i < lfsr.size(); ++i) {
				lfsr[i].Val(initial_value);
				lfsr[i].Poly(lfsr_poly[i]);
				lfsr[i].Outpin(lfsr_output_pin);
			}
			lfsr[0].ResetPeriod(8190);
			lfsr[0].ResetVal(initial_value);

			for (std::size_t i = 0; i < SecondLfsrPhase(sv_number); ++i)
				lfsr[1].Shift();

			for (std::size_t i = 0; i < code_len; ++i) {
				*prn++ = static_cast<T>(1 - 2 * (lfsr[0].Get() ^ lfsr[1].Get()));
				for (auto& el : lfsr)
					el.Shift();
			}
		}
	};
}
