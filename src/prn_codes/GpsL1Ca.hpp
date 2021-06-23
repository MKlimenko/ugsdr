#pragma once

#include "codegen.hpp"

#include <array>

namespace ugsdr {
	class GpsL1Ca final : public Codegen<GpsL1Ca> {
	private:
		static constexpr std::size_t code_len = 1023;
		static constexpr std::size_t initial_value = 0x3FF;
		static constexpr std::array<std::uint32_t, 2> lfsr_poly = { 0x204, 0x3A6 } ;
		static constexpr std::size_t lfsr_output_pin = 9;

		static std::size_t XbPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				5,   6,   7,   8,   17,  18,  139, 140, 141, 251,
				252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
				473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
				861, 862, 863, 950, 947, 948, 950
			};

			if (sv_number > delay_table.size())
				throw std::runtime_error("SV number for the GPS C/A code exceed available values");

			return 1023 - delay_table[sv_number];
		}
	protected:
		friend class Codegen<GpsL1Ca>;

		static auto GetCodeLength() {
			return code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			std::array<Lfsr, 2> lfsr;
			for (std::size_t i = 0; i < lfsr.size(); ++i) {
				lfsr[i].Val(initial_value);
				lfsr[i].Poly(lfsr_poly[i]);
				lfsr[i].Outpin(lfsr_output_pin);
			}

			for (std::size_t i = 0; i < XbPhase(sv_number); ++i)
				lfsr[1].Shift();

			for (std::size_t i = 0; i < lfsr[0].CountSequenceLen(); ++i) {
				*prn++ = static_cast<T>(1 - 2 * (lfsr[0].Get() ^ lfsr[1].Get()));
				for (auto& el : lfsr)
					el.Shift();
			}
		}
	};
}
