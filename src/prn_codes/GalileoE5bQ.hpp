#pragma once

#include "codegen.hpp"
#include "GalileoE5.hpp"

#include <array>
#include <string>

namespace ugsdr {
	class GalileoE5bQ final : public Codegen<GalileoE5bQ>, public GalileoE5< (064021 >> 1), (043143 >> 1)> {
	private:
		static constexpr std::size_t code_len = 10230;

		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				4091,	11240,	1008,	2665,	1441,	3477,	13174,	15168,	9331,	8554,	13204,	10716,
				252,	2468,	7038,	10223,	6803,	13678,	11831,	1509,	14770,	2143,	7379,	140,
				6334,	5858,	15443,	11193,	5867,	15682,	4159,	6627,	10068,	3224,	4384,	9934,
				9548,	14796,	1279,	13418,	10480,	2920,	11764,	3100,	3246,	14953,	3035,	7216,	15391,	16114
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the Galileo E5aI code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<GalileoE5bQ>;

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
