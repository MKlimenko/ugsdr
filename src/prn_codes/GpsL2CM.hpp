#pragma once

#include "codegen.hpp"
#include "L2CM.hpp"

namespace ugsdr {
	class GpsL2CM final : public Codegen<GpsL2CM>, public L2CM {
	private:
		static auto LfsrValue(std::size_t sv_number) {
			constexpr static auto lfsr_values = std::array{
				22999285,	84902174,	17716541,	23790168,	68613052,	106676766,	16105048,	76969855,
				77965538,	57558230,	99108613,	54913561,	110517083,	71826178,	277102,		56514710,
				50699741,	96562474,	17596635,	29840564,	105919054,	133910036,	122553487,	6698247,
				33064953,	76005959,	100494732,	34085073,	32302080,	128891171,	106345879,	100852055,
			};

			if (sv_number >= lfsr_values.size())
				throw std::runtime_error("SV number for the GPS L2CM code exceed available values");

			return static_cast<std::size_t>(lfsr_values[sv_number]);
		}

	protected:
		friend class Codegen<GpsL2CM>;

		template <typename T>
		static void Generate(T* prn, std::size_t sv_number) {
			L2CM::Generate(prn, sv_number, LfsrValue);
		}
	};
}
