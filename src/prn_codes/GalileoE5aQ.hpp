#pragma once

#include "codegen.hpp"
#include "GalileoE5a.hpp"

#include <array>
#include <string>

namespace ugsdr {
	class GalileoE5aQ final : public Codegen<GalileoE5aQ>, public GalileoE5a {
	private:
		static constexpr std::size_t code_len = 10230;

		static std::size_t SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				9429,	7251,	11349,	5127,	10136,	6507,	13047,	10543,	16258,	11684,	10019,	14529,
				5499,	9822,	5382,	8283,	10936,	10741,	3372,	4469,	11078,	7249,	14380,	1348,
				8116,	6124,	2287,	99,	16023,	1471,	5340,	16324,	12693,	11908,	9371,	4782,	8762,
				8084,	15792,	4162,	5988,	15478,	7013,	1776,	13009,	2343,	13561,	4993,	11991,	2426
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the Galileo E5aI code exceed available values");

			return delay_table[sv_number];
		}

	protected:
		friend class Codegen<GalileoE5aQ>;

		static auto NumberOfMilliseconds() {
			return 1;
		}

		static auto GetCodeLength() {
			return code_len;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			GalileoE5a::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
