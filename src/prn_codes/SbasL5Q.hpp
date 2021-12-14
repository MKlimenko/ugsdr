#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class SbasL5Q final : public Codegen<SbasL5Q>, public L5 {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			sv_number -= ugsdr::sbas_sv_offset;
			constexpr static auto delay_table = std::array{
				6837,	1393,	7383,	611,	4920,	5416,	1611,	2474,	118,
				1382,	1092,	7950,	7223,	1769,	4721,	1252,	5147,	2165,
				7897,	4054,	3498,	6571
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the SBAS L5Q code exceed available values");

			return static_cast<std::size_t>(delay_table[sv_number]);
		}

	protected:
		friend class Codegen<SbasL5Q>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
