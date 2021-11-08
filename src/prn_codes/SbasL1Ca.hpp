#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "GoldCodes.hpp"

namespace ugsdr {
	class SbasL1Ca final : public Codegen<SbasL1Ca>, public GoldCodes {
	private:
		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			sv_number -= ugsdr::sbas_sv_offset;

			constexpr static auto delay_table = std::array{
				145,  175,  52,   21,   237,  235,  886,  657,   634,  762,
				355,  1012, 176,  603,  130,  359,  595,  68,    386,  797,
				456,  499
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the SBAS C/A code exceed available values");

			return 1023 - delay_table[sv_number];
		}

	protected:
		friend class Codegen<SbasL1Ca>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GoldCodes::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
