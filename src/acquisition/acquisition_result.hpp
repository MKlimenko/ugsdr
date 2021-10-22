#pragma once

#include "../common.hpp"
#include <vector>

namespace ugsdr {
	template <typename T>
	struct AcquisitionResult final {
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

		template <typename Archive>
		void save(Archive& ar) const {
			ar(
				CEREAL_NVP(sv_number),
				CEREAL_NVP(doppler),
				CEREAL_NVP(code_offset),
				CEREAL_NVP(level),
				CEREAL_NVP(sigma),
				CEREAL_NVP(intermediate_frequency),
				CEREAL_NVP(output_peak)
			);
		}

		template <typename Archive>
		void load(Archive& ar) {
			ar(
				sv_number,
				doppler,
				code_offset,
				level,
				sigma,
				intermediate_frequency,
				(output_peak)
			);
		}
	};
}
