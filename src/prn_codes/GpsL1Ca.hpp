#pragma once

#include "codegen.hpp"
#include "GoldCodes.hpp"

namespace ugsdr {
	class GpsL1Ca final : public Codegen<GpsL1Ca>, public GoldCodes {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				5,   6,   7,   8,   17,  18,  139, 140, 141, 251,
				252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
				473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
				861, 862, 863, 950, 947, 948, 950
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the GPS C/A code exceed available values");

			return static_cast<std::size_t>(1023 - delay_table[sv_number]);
		}

	protected:
		friend class Codegen<GpsL1Ca>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GoldCodes::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
