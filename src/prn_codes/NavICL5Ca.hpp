#pragma once

#include "codegen.hpp"
#include "GoldCodes.hpp"

namespace ugsdr {
	class NavICL5Ca final : public Codegen<NavICL5Ca>, public GoldCodes {
	private:
		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array {
				575,	644,	881,	840,	750,	386,	53,	284,	223,	152,	704,	781,	12,	23
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the NavIC L5 C/A code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<NavICL5Ca>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GoldCodes::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
