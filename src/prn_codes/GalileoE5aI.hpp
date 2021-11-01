#pragma once

#include "codegen.hpp"
#include "GalileoE5.hpp"

#include <array>
#include <string>

namespace ugsdr {
	class GalileoE5aI final : public Codegen<GalileoE5aI>, public GalileoE5< (040503 >> 1), (050661 >> 1)> {
	private:
		static constexpr std::size_t code_len = 10230;

		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				1344, 6591, 5065, 15628, 1178, 3984, 3040, 3487, 5104, 5116, 9197, 10580, 8058, 4124, 6713, 13753, 8560,
				12600, 16190, 13938, 11820, 5059, 8075, 10769, 6365, 14657, 10084, 16301, 183, 3320, 12010, 1313, 3988,
				7300, 10301, 15760, 12358, 9918, 13182, 12111, 4730, 6076, 13065, 4968, 15724, 4466, 14946, 9079, 4406,
				11750,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the Galileo E5aI code exceed available values");

			return delay_table[sv_number];
		}		
		
	protected:
		friend class Codegen<GalileoE5aI>;

		static auto NumberOfMilliseconds() {
			return 1;
		}

		static auto GetCodeLength() {
			return code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GalileoE5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
