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

		std::vector<std::complex<float>> prompt;
		
		TrackingParameters(const AcquisitionResult& acquisition, double sampling_frequency) :	code_phase(acquisition.code_offset),
																								carrier_frequency(acquisition.doppler),
																								intermediate_frequency(acquisition.intermediate_frequency),
																								sv(acquisition.sv_number),
																								sampling_rate(sampling_frequency)	{
			switch (sv.system) {
			case System::Gps:
				code_frequency = 1.023e6;
				base_code_frequency = 1.023e6;
				break;
			case System::Glonass:
				code_frequency = 0.511e6;
				base_code_frequency = 0.511e6;
				break;
			default:
				break;
			}

			prompt.reserve(10000);
		}

		auto GetSamplesPerChip() const {
			return sampling_rate / base_code_frequency;
		}
	};
}
