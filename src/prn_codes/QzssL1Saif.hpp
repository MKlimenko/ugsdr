#pragma once

#include "codegen.hpp"
#include "GoldCodes.hpp"

namespace ugsdr {
	class QzssL1Saif final : public Codegen<QzssL1Saif>, public GoldCodes {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				144,	476,	193,	109,	445,	291,	87,		399,	292,
				144,	476,	193,	109,	445,	291,	87,		399,	292,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the QZSS SAIF code exceed available values");

			return static_cast<std::size_t>(1023 - delay_table[sv_number]);
		}

	protected:
		friend class Codegen<QzssL1Saif>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GoldCodes::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
