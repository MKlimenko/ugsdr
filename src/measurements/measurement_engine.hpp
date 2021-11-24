#pragma once

#include "observable.hpp"
#include "timescale.hpp"
#include "../tracking/tracker.hpp"
#include "../tracking/tracking_parameters.hpp"
#include "../helpers/rtklib_helpers.hpp"

#include <fstream>

namespace ugsdr {
	class MeasurementEngine final {
	private:
		void FillObservable(const ugsdr::Observable& current_observable, const gtime_t& time, std::size_t epoch, obsd_t& current_rtklib_data) {
			current_rtklib_data.time = time;
			current_rtklib_data.sat = rtklib_helpers::ConvertSv(current_observable.sv);
			current_rtklib_data.code[0] = rtklib_helpers::ConvertCode(current_observable.sv);
			current_rtklib_data.P[0] = current_observable.pseudorange[epoch] / 1000.0 * CLIGHT;
			current_rtklib_data.L[0] = current_observable.pseudophase[epoch];
			current_rtklib_data.D[0] = current_observable.doppler[epoch];
			current_rtklib_data.SNR[0] = current_observable.snr[epoch] * 1000;
		}

		void FillEphemeris(const ugsdr::GpsEphemeris& current_ephemeris, std::uint8_t sat, eph_t& current_rtklib_ephemeris) {
			current_rtklib_ephemeris.sat = sat;
			current_rtklib_ephemeris.iode = current_ephemeris.iodc;
			current_rtklib_ephemeris.iodc = current_ephemeris.iodc;
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
	
		void InitNav(nav_t* ptr) { // revise for multiple gnss
			if (!ptr)
				throw std::runtime_error("Unexpected nullptr");

			ptr->n = observables.size();
			ptr->nmax = ptr->n;
			ptr->eph = new eph_t[ptr->n];
		}

		static void FreeNav(nav_t* ptr) {
			if (!ptr)
				return;
			if (ptr->eph)
				delete[] ptr->eph;
		}

	public:
		TimeScale receiver_time_scale;
		std::vector<Observable> observables;
		std::unique_ptr<nav_t, void(*)(nav_t*)> nav;

		template <typename T>
		MeasurementEngine(const ugsdr::Tracker<T>& tracker) : MeasurementEngine(tracker.GetTrackingParameters()) {}

		template <typename T>
		MeasurementEngine(const std::vector<ugsdr::TrackingParameters<T>>& tracking_results) : nav(new nav_t(), FreeNav) {
			if (tracking_results.empty())
				throw std::runtime_error("Empty tracking results");

			receiver_time_scale = TimeScale(tracking_results.begin()->prompt.size());
			observables.reserve(tracking_results.size());

			for (auto& el : tracking_results) {
				auto current_observable = Observable::MakeObservable(el, receiver_time_scale);
				if (current_observable)
					observables.push_back(current_observable.value());
			}

			auto day_offset = 0;
			for (auto& obs : observables) {
				if (obs.sv.system == ugsdr::System::Gps) {
					day_offset = std::floor(std::get<GpsEphemeris>(obs.ephemeris).tow / (24 * 60 * 60));
					break;
				}
			}

			for (auto& obs : observables)
				obs.UpdatePseudoranges(day_offset);

			InitNav(nav.get());
			for (std::size_t i = 0; i < observables.size(); ++i) 
				FillEphemeris(std::get<ugsdr::GpsEphemeris>(observables[i].ephemeris), rtklib_helpers::ConvertSv(observables[i].sv), nav->eph[i]);
		
		}
	
		auto GetMeasurementEpoch(std::size_t epoch) {
			auto obs = std::vector<obsd_t>(observables.size(), { {0} });
			for (std::size_t i = 0; i < obs.size(); ++i) 
				FillObservable(observables[i], gpst2time(nav->eph[i].week, observables[i].time_scale[epoch] * 1e-3), epoch, obs[i]);
			
			return std::make_pair(std::move(obs), nav.get());
		}

		void WriteRinex(std::size_t epoch_step = 1) {
			auto rinex_obs = std::unique_ptr<FILE, int(*)(FILE*)>(fopen("d:\\tmp\\rinex.obs", "w"), fclose);

			auto rnxopt = std::make_unique<rnxopt_t>();
			rnxopt->rnxver = 303;
			rnxopt->ttol = epoch_step * 0.001;
			rnxopt->tstart = gpst2time(nav->eph[0].week, receiver_time_scale.first() * 1e-3);
			rnxopt->tend = gpst2time(nav->eph[0].week, receiver_time_scale.last() * 1e-3);
			rnxopt->navsys = SYS_GPS;
			rnxopt->nobs[0] = 4;
			rnxopt->obstype = OBSTYPE_ALL;
			rnxopt->freqtype = FREQTYPE_ALL;

			// move to rtklib_helpers
			std::strcpy(rnxopt->tobs[0][0], "C1C");
			std::strcpy(rnxopt->tobs[0][1], "L1C");
			std::strcpy(rnxopt->tobs[0][2], "D1C");
			std::strcpy(rnxopt->tobs[0][3], "S1C");

			auto status_header = outrnxobsh(rinex_obs.get(), rnxopt.get(), nav.get());
		
			for (std::size_t epoch = 0; epoch < receiver_time_scale.length(); epoch += epoch_step) {
				auto [obs, nav] = GetMeasurementEpoch(epoch);
				auto status_body = outrnxobsb(rinex_obs.get(), rnxopt.get(), obs.data(), static_cast<int>(obs.size()), 0);
			}
		}
	};
}
