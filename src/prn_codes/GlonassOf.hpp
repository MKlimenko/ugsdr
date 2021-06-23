#pragma once

#include "codegen.hpp"

namespace ugsdr {
	class GlonassOf final : public Codegen<GlonassOf> {
	private:
		static constexpr std::size_t code_len = 511;
		static constexpr std::size_t initial_value = 0x1FF;
		static constexpr std::size_t lfsr_poly = 0x110;
		static constexpr std::size_t lfsr_output_pin = 6;

	protected:
		friend class Codegen<GlonassOf>;

		template <typename T>
		static void Generate(T* prn) {
			Lfsr lfsr;
			lfsr.Val(initial_value);
			lfsr.Poly(lfsr_poly);
			lfsr.Outpin(lfsr_output_pin);

			for (std::size_t i = 0; i < lfsr.CountSequenceLen(); ++i) {
				*prn++ = static_cast<T>(1 - 2 * lfsr.Get());
				lfsr.Shift();
			}
		}
	};
}
