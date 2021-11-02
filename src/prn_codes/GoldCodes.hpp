#pragma once

#include "codegen.hpp"

#include <array>

namespace ugsdr {
	class GoldCodes {
	private:
		static constexpr std::size_t code_len = 1023;
		static constexpr std::size_t initial_value = 0x3FF;
		static constexpr std::array<std::uint32_t, 2> lfsr_poly = { 0x204, 0x3A6 } ;
		static constexpr std::size_t lfsr_output_pin = 9;

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

			for (std::size_t i = 0; i < SecondLfsrPhase(sv_number); ++i)
				lfsr[1].Shift();

			for (std::size_t i = 0; i < lfsr[0].CountSequenceLen(); ++i) {
				*prn++ = static_cast<T>(1 - 2 * (lfsr[0].Get() ^ lfsr[1].Get()));
				for (auto& el : lfsr)
					el.Shift();
			}
		}
	};
}
