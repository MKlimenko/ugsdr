#pragma once

#include "codegen.hpp"

#include <array>
#include <stdexcept>

namespace ugsdr {
	template <std::uint32_t lfsr_val_1, std::uint32_t lfsr_val_2>
	class GalileoE5 {
	private:
		static constexpr std::size_t code_len = 10230;
		static constexpr std::size_t initial_value = 0xFFFFFFFF;
		static constexpr std::array<std::uint32_t, 2> lfsr_poly = { lfsr_val_1, lfsr_val_2 };
		static constexpr std::size_t lfsr_output_pin = 13;
		
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

			lfsr[1].ShiftCounter(0);
			for (auto& el : lfsr) 
				el.ResetPeriod(10230);
			

			for (std::size_t i = 0; i < lfsr[0].CountSequenceLen(); ++i) {
				*prn++ = static_cast<T>(1 - 2 * (lfsr[0].Get() ^ lfsr[1].Get()));
				for (auto& el : lfsr)
					el.Shift();
			}
		}
	};
}
