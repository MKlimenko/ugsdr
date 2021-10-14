#pragma once

#include "../common.hpp"
#include <vector>

namespace ugsdr {
	template <typename T>
	struct AcquisitionResult {
		Sv sv_number;
		double doppler = 0;
		double code_offset = 0;
		double level = 0;
		double sigma = 1.0;
		// satellite_system;
		double intermediate_frequency = 0;
		std::vector<T> output_peak;
		
		auto operator<=>(const AcquisitionResult& rhs) const {
			return level <=> rhs.level;
		}

		auto GetSnr() const {
			return level / sigma;
		}

		auto GetAcquiredSignalType() const {
			switch (sv_number.system) {
			case System::Gps:
				return Signal::GpsCoarseAcquisition_L1;
			case System::Glonass: 
				return Signal::GlonassCivilFdma_L1;
			default:
				throw std::runtime_error("Unexpected satellite system from acquisition");
			}
		}
	};
}
