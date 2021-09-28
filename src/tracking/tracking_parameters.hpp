#pragma once

#include "../common.hpp"
#include "../acquisition/acquisition_result.hpp"

namespace ugsdr {
	struct TrackingParameters {
		double code_phase = 0.0;
		double code_frequency = 0.0;
        double base_code_frequency = 0.0;

        double carrier_phase = 0.0;
        double carrier_frequency = 0.0;
        double intermediate_frequency = 0.0;
        double sampling_rate = 0.0;
        Sv sv;

		TrackingParameters(const AcquisitionResult& acquisition) :	code_phase(acquisition.code_offset), carrier_phase(acquisition.doppler),
																	intermediate_frequency(acquisition.intermediate_frequency), sv(acquisition.sv_number) {
			switch (sv.system) {
			case System::Gps:
				break;
			case System::Glonass:
				break;
			default:
				break;
			}
		}
	};
}
