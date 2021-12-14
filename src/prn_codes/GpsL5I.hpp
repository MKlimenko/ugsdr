#pragma once

#include "codegen.hpp"
#include "L5.hpp"

namespace ugsdr {
	class GpsL5I final : public Codegen<GpsL5I>, public L5 {
	private:
		static auto SecondLfsrPhase(std::size_t sv_number) {
			constexpr static auto delay_table = std::array{
				266,	365,	804,	1138,	1509,	1559,	1756,	2084,	2170,
				2303,	2527,	2687,	2930,	3471,	3940,	4132,	4332,	4924,
				5343,	5443,	5641,	5816,	5898,	5918,	5955,	6243,	6345,	
				6477,	6518,	6875,	7168,	7187
			};

			if (sv_number >= delay_table.size())
				throw std::runtime_error("SV number for the GPS L5I code exceed available values");

			return static_cast<std::size_t>(delay_table[sv_number]);
		}

	protected:
		friend class Codegen<GpsL5I>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L5::Generate(prn, sv_number, SecondLfsrPhase);
		}
	};
}
