#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class SbasL5I final : public Codegen<SbasL5I>, public L5 {
	private:
		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			sv_number -= ugsdr::sbas_sv_offset;
			constexpr static auto delay_table = std::array{
				2797,	934,	3023,	3632,	1330,	4909,	4867,	1183,	3990,
				6217,	1224,	1733,	2319,	3928,	2380,	841,	5049,	7027,
				1197,	7208,	8000,	152,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the SBAS L5I code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<SbasL5I>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
