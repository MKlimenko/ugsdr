#pragma once

#include "../common.hpp"

namespace ugsdr {
	struct AcquisitionResult {
		Sv sv_number;
		double doppler = 0;
		double code_offset = 0;
		double level = 0;
		double sigma = 1.0;
		// satellite_system;
		double intermediate_frequency = 0;

		auto operator<=>(const AcquisitionResult& rhs) const {
			return level <=> rhs.level;
		}

		auto GetSnr() const {
			return level / sigma;
		}
	};
}
