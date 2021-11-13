#pragma once

#include "../helpers/rtklib_helpers.hpp"
#include "standalone_engine.hpp"

#include "../../external/plusifier/Plusifier.hpp"

#if 0
gtime_t time;       /* receiver sampling time (GPST) */
uint8_t sat, rcv;    /* satellite/receiver number */
uint16_t SNR[NFREQ + NEXOBS]; /* signal strength (0.001 dBHz) */
uint8_t  LLI[NFREQ + NEXOBS]; /* loss of lock indicator */
uint8_t code[NFREQ + NEXOBS]; /* code indicator (CODE_???) */
double L[NFREQ + NEXOBS]; /* observation data carrier-phase (cycle) */
double P[NFREQ + NEXOBS]; /* observation data pseudorange (m) */
float  D[NFREQ + NEXOBS]; /* observation data doppler frequency (Hz) */
#endif

namespace ugsdr {
	class StandaloneRtklib final : public StandaloneEngine<StandaloneRtklib> {
	protected:
		friend class StandaloneEngine<StandaloneRtklib>;

		plusifier::PointerWrapper<raw_t, free_raw> raw;

		auto FillRtklibData(std::size_t epoch) {
			for (std::size_t i = 0; i < measurement_engine.observables.size(); ++i) {
				auto& current_rtklib_data = raw->obs.data[i];
				auto& current_observable = measurement_engine.observables[i];
				current_rtklib_data.sat = rtklib_helpers::ConvertSv(current_observable.sv);
				current_rtklib_data.code[0] = rtklib_helpers::ConvertCode(current_observable.sv);
			}
		}

		// L1 for now
		auto Estimate(std::size_t epoch) {
			FillRtklibData(epoch);
			auto a = 5;
			return 0;
		}

	public:
		StandaloneRtklib(MeasurementEngine& measurements) : StandaloneEngine<StandaloneRtklib>(measurements), raw(new raw_t) {
			init_raw(raw, STRFMT_BINEX);
			raw->obs.nmax = MAXOBS;
			raw->obs.n = 1;
		}
	};
}