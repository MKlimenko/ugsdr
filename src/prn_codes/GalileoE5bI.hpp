#pragma once

#include "codegen.hpp"
#include "GalileoE5.hpp"

#include <array>
#include <string>

namespace ugsdr {
	class GalileoE5bI final : public Codegen<GalileoE5bI>, public GalileoE5< (064021 >> 1), (051445 >> 1)> {
	private:
		static constexpr std::size_t code_len = 10230;

		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				1885,	15693,	11828,	12777,	6107,	4456,	11498,	11259,	1386,	3047,	11008,	8649,
				11983,	8855,	11527,	4778,	14687,	12597,	11871,	7960,	1883,	4943,	2305,	4813,
				10621,	9544,	1378,	12025,	4306,	5555,	5262,	5402,	5044,	9329,	14670,	3360,
				14974,	69,	3100,	2703,	8928,	5333,	7860,	5517,	10551,	4117,	1041,	3082,	11618,	9005,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the Galileo E5aI code exceed available values");

			return static_cast<std::size_t>(delay_table[sv_number]);
		}		
		
	protected:
		friend class Codegen<GalileoE5bI>;

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
