#pragma once

#include "codegen.hpp"
#include "Weil.hpp"

namespace ugsdr {
	class BeiDouB1C final : public Codegen<BeiDouB1C>, public Weil<10243> {
	private:
		static constexpr std::size_t code_len = 10230;

		static auto SecondLegendrePhase(std::size_t sv_number) {
			constexpr static auto phase_table = std::array{
				2678, 4802,  958,  859, 3843,
				2232,  124, 4352, 1816, 1126,
				1860, 4800, 2267,  424, 4192,
				4333, 2656, 4148,  243, 1330,
				1593, 1470,  882, 3202, 5095,
				2546, 1733, 4795, 4577, 1627,
				3638, 2553, 3646, 1087, 1843,
				216 , 2245,  726, 1966,  670,
				4130,   53, 4830,  182, 2181,
				2006, 1080, 2288, 2027,  271,
				915,  497,  139, 3693, 2054,
				4342, 3342, 2592, 1007,  310,
				4203,  455, 4318
			};

			if (sv_number >= phase_table.size())
				throw std::runtime_error("SV number for the BeiDou B1C code exceed available values");

			return static_cast<std::size_t>(phase_table[sv_number]);
		}

		static auto WeilTruncationPoint(std::size_t sv_number) {
			constexpr static auto truncation_point_table = std::array{
				699,  694, 7318, 2127,  715,
				6682, 7850, 5495, 1162, 7682,
				6792, 9973, 6596, 2092,   19,
				10151, 6297, 5766, 2359, 7136,
				1706, 2128, 6827,  693, 9729,
				1620, 6805,  534,  712, 1929,
				5355, 6139, 6339, 1470, 6867,
				7851, 1162, 7659, 1156, 2672,
				6043, 2862,  180, 2663, 6940,
				1645, 1582,  951, 6878, 7701,
				1823, 2391, 2606,  822, 6403,
				239,  442, 6769, 2560, 2502,
				5072, 7268,341
			};

			if (sv_number >= truncation_point_table.size())
				throw std::runtime_error("SV number for the BeiDou B1C code exceed available values");

			return static_cast<std::size_t>(truncation_point_table[sv_number] - 1);
		}		

	protected:
		friend class Codegen<BeiDouB1C>;

		static auto NumberOfMilliseconds() {
			return 10;
		}

		static auto GetCodeLength() {
			return code_len * 2;
		}

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			Weil::GenerateBoc<code_len>(prn, sv_number, SecondLegendrePhase, WeilTruncationPoint);
		}
	};
}
