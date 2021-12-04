#pragma once

#include "../common.hpp"
#include <vector>

#ifdef HAS_CEREAL
#include <cereal/cereal.hpp>
#endif

namespace ugsdr {
	template <typename T>
	struct AcquisitionResult final {
		Sv sv_number;
		double doppler = 0;
		double code_offset = 0;
		double level = 0;
		double sigma = 1.0;
		double intermediate_frequency = 0;
		std::vector<T> output_peak;
		
		auto operator<=>(const AcquisitionResult& rhs) const {
			return level <=> rhs.level;
		}

		auto GetSnr() const {
			return level / sigma;
		}

		auto GetAcquiredSignalType() const {
			return sv_number.signal;
		}

		template <typename Archive>
		void save(Archive& ar) const {
#ifdef HAS_CEREAL
			ar(
				CEREAL_NVP(sv_number),
				CEREAL_NVP(doppler),
				CEREAL_NVP(code_offset),
				CEREAL_NVP(level),
				CEREAL_NVP(sigma),
				CEREAL_NVP(intermediate_frequency),
				CEREAL_NVP(output_peak)
			);
#endif
		}

		template <typename Archive>
		void load(Archive& ar) {
#ifdef HAS_CEREAL
			ar(
				sv_number,
				doppler,
				code_offset,
				level,
				sigma,
				intermediate_frequency,
				(output_peak)
			);
#endif
		}
	};
}
