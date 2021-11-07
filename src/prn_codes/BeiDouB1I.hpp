#pragma once

#include "codegen.hpp"

namespace ugsdr {
	class BeiDouB1I final : public Codegen<BeiDouB1I> {
	private:
		static constexpr std::size_t code_len = 2046;
		static constexpr std::size_t initial_value = 0x2AA;
		static constexpr std::array<std::uint32_t, 2> lfsr_poly = { 0x7C1, 0x59F };
		static constexpr std::size_t lfsr_output_pin = 10;


		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				712,	1581,	1414,	1550,	581,	771,	1311,	1043,	1549,	359,
				710,	1579,	1548,	1103,	579,	769,	358,	709,	1411,	1547,
				1102,	578,	357,	1577,	1410,	1546,	1101,	707,	1576,	1409,
				1545,	354,	705,	1574,	353,	704,	352,	1828,	1525,	1775,
				26,		452,	1386,	615,	1178,	1982,	923,	1427,	1493,	1362,
				1735,	1022,	1907,	697,	1331,	1964,	1228,	1826,	1523,	1773,
				24,	1651,	1785,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the BeiDOu B1I code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<BeiDouB1I>;

		static auto NumberOfMilliseconds() {
			return 1;
		}

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
				lfsr[i].ResetVal(initial_value);
				lfsr[i].ResetPeriod(code_len);
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
