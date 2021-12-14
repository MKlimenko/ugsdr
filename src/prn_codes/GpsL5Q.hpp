#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class GpsL5Q final : public Codegen<GpsL5Q>, public L5 {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				1701,	323,	5292,	2020,	5429,	7136,	1041,	5947,	4315,
				148,	535,	1939,	5206,	5910,	3595,	5135,	6082,	6990,
				3546,	1523,	4548,	4484,	1893,	3961,	7106,	5299,	4660,
				276,	4389,	3783,	1591,	1601,
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the GPS L5Q code exceed available values");

			return static_cast<std::size_t>(delay_table[sv_number]);
		}

	protected:
		friend class Codegen<GpsL5Q>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
