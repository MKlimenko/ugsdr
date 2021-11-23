#pragma once

#include "../helpers/rtklib_helpers.hpp"
#include "standalone_engine.hpp"

#include "../../external/plusifier/Plusifier.hpp"

namespace ugsdr {
	class StandaloneRtklib final : public StandaloneEngine<StandaloneRtklib> {
	protected:
		friend class StandaloneEngine<StandaloneRtklib>;

		plusifier::PointerWrapper<raw_t, free_raw> raw;

		void FillObservables(const ugsdr::Observable& current_observable, const gtime_t& time, std::size_t epoch, obsd_t& current_rtklib_data) {
			current_rtklib_data.time = time;
			current_rtklib_data.sat = rtklib_helpers::ConvertSv(current_observable.sv);
			current_rtklib_data.code[0] = rtklib_helpers::ConvertCode(current_observable.sv);
			current_rtklib_data.P[0] = current_observable.pseudorange[epoch] / 1000.0 * CLIGHT;
			current_rtklib_data.L[0] = current_observable.pseudophase[epoch];
			current_rtklib_data.D[0] = current_observable.doppler[epoch];
			current_rtklib_data.SNR[0] = 45 * 1000; // for now;
			raw->obs.n++;
		}

		void FillEphemeris(const ugsdr::GpsEphemeris& current_ephemeris, std::uint8_t sat, eph_t& current_rtklib_ephemeris) {
			current_rtklib_ephemeris.sat = sat;
			current_rtklib_ephemeris.iode = current_ephemeris.iodc;
			current_rtklib_ephemeris.iode = current_ephemeris.iodc;
			current_rtklib_ephemeris.sva = current_ephemeris.accuracy;
			current_rtklib_ephemeris.svh = current_ephemeris.health;
			current_rtklib_ephemeris.week = current_ephemeris.week_number;
			current_rtklib_ephemeris.code = CODE_NONE;
			current_rtklib_ephemeris.flag = 0;
			current_rtklib_ephemeris.toe = gpst2time(static_cast<int>(current_ephemeris.week_number), current_ephemeris.toe);
			current_rtklib_ephemeris.toc = gpst2time(static_cast<int>(current_ephemeris.week_number), current_ephemeris.toc);
			current_rtklib_ephemeris.ttr = gpst2time(static_cast<int>(current_ephemeris.week_number), current_ephemeris.toe);
			current_rtklib_ephemeris.A = current_ephemeris.sqrt_A * current_ephemeris.sqrt_A;
			current_rtklib_ephemeris.e = current_ephemeris.e;
			current_rtklib_ephemeris.i0 = current_ephemeris.i_0;
			current_rtklib_ephemeris.OMG0 = current_ephemeris.omega_0;
			current_rtklib_ephemeris.omg = current_ephemeris.omega;
			current_rtklib_ephemeris.M0 = current_ephemeris.m0;
			current_rtklib_ephemeris.deln = current_ephemeris.delta_n;
			current_rtklib_ephemeris.OMGd = current_ephemeris.omega_dot;
			current_rtklib_ephemeris.idot = current_ephemeris.i_dot;
			current_rtklib_ephemeris.crc = current_ephemeris.crc;
			current_rtklib_ephemeris.crs = current_ephemeris.crs;
			current_rtklib_ephemeris.cuc = current_ephemeris.cuc;
			current_rtklib_ephemeris.cus = current_ephemeris.cus;
			current_rtklib_ephemeris.cic = current_ephemeris.cic;
			current_rtklib_ephemeris.cis = current_ephemeris.cis;
			current_rtklib_ephemeris.toes = current_ephemeris.toe;
			current_rtklib_ephemeris.fit = 4;
			current_rtklib_ephemeris.f0 = current_ephemeris.af0;
			current_rtklib_ephemeris.f1 = current_ephemeris.af1;
			current_rtklib_ephemeris.f2 = current_ephemeris.af2;
			current_rtklib_ephemeris.tgd[0] = current_ephemeris.group_delay;
		}

		auto FillRtklibData(std::size_t epoch) {
			auto& obs = raw->obs;
			auto& nav = raw->nav;
			obs.n = 0;
			nav.n = 0;
			for (std::size_t i = 0; i < measurement_engine.observables.size(); ++i) {
				auto& current_rtklib_data = obs.data[i];
				const auto& current_observable = measurement_engine.observables[i];
				const auto& current_ephemeris = std::get<GpsEphemeris>(current_observable.ephemeris);
				
				auto obs_time = gpst2time(static_cast<int>(current_ephemeris.week_number), current_observable.time_scale[epoch] * 1e-3);
				FillObservables(current_observable, obs_time, epoch, current_rtklib_data);

				auto& current_rtklib_ephemeris = nav.eph[i];
				FillEphemeris(current_ephemeris, current_rtklib_data.sat, current_rtklib_ephemeris);

				nav.n++;
			}
		}

		// L1 for now
		auto Estimate(std::size_t epoch) {
			FillRtklibData(epoch);

			std::vector<double> azel((raw->obs.n) * 2);
			std::vector<ssat_t> ssat(MAXSAT);
			auto sol = std::make_unique<sol_t>();
			char msg[128] = "";
			auto status = pntpos(raw->obs.data, raw->obs.n, &raw->nav, &prcopt_default, sol.get(), azel.data(), ssat.data(), msg);

			if (!status)
				std::cout << "No solution: " << msg << std::endl;
			else {
				std::cout << "Valid solution:" << std::endl;
				std::cout << "\tNumber of satellites: " << static_cast<int>(sol->ns) << std::endl;
				std::cout << "\tCoordinates and time: ";
				for (std::size_t j = 0; j < 3; ++j)
					std::cout << std::fixed << sol->rr[j] << " ";
				std::cout << std::fixed << sol->dtr[0] << std::endl;
			}

			return 0;
		}

	public:
		StandaloneRtklib(MeasurementEngine& measurements) : StandaloneEngine<StandaloneRtklib>(measurements), raw(new raw_t()) {
			init_raw(raw, STRFMT_BINEX);

			raw->obs.nmax = MAXOBS;
			raw->nav.nmax = MAXSAT * 2;
		}
	};
}