#pragma once

#include "codegen.hpp"
#include "GoldCodes.hpp"

namespace ugsdr {
	class QzssL1Ca final : public Codegen<QzssL1Ca>, public GoldCodes {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				339,	208,	711,	189,	263,	537,	663,	942,	173,	900,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the QZSS C/A code exceed available values");

			return static_cast<std::size_t>(1023 - delay_table[sv_number]);
		}

	protected:
		friend class Codegen<QzssL1Ca>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GoldCodes::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
