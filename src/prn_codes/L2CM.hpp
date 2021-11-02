#pragma once

#include "codegen.hpp"

namespace ugsdr {
	class L2CM {
	private:
		static constexpr std::size_t code_len = 10230;
		static constexpr std::uint32_t lfsr_poly = 0x494953C;
		static constexpr std::size_t lfsr_output_pin = 26;

	protected:
		static auto NumberOfMilliseconds() {
			return 20;
		}

		static auto GetCodeLength() {
			return code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number, std::size_t(*LfsrValue)(std::size_t)) {
			Lfsr lfsr;
			lfsr.Val(LfsrValue(sv_number));
			lfsr.Poly(lfsr_poly);
			lfsr.Outpin(lfsr_output_pin);
			lfsr.ResetPeriod(10230);

			for (std::size_t i = 0; i < lfsr.CountSequenceLen(); ++i) {
				*prn++ = static_cast<T>(1 - 2 * lfsr.Get());
				lfsr.Shift();
			}
		}
	};
}
