#pragma once

#include "observable.hpp"
#include "timescale.hpp"
#include "../tracking/tracker.hpp"
#include "../tracking/tracking_parameters.hpp"
#include "../helpers/rtklib_helpers.hpp"

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

		template <typename T>
		MeasurementEngine(const ugsdr::Tracker<T>& tracker) : MeasurementEngine(tracker.GetTrackingParameters()) {}

		template <typename T>
		MeasurementEngine(const std::vector<ugsdr::TrackingParameters<T>>& tracking_results) {
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
		}
	
		auto GetMeasurementEpoch(std::size_t epoch) {
			auto nav = std::unique_ptr<nav_t, void(*)(nav_t*)>(new nav_t(), FreeNav);
			InitNav(nav.get());

			auto obs = std::vector<obsd_t>(observables.size(), { {0} });
			for (std::size_t i = 0; i < obs.size(); ++i) {
				const auto& current_observable = observables[i];
				FillEphemeris(std::get<ugsdr::GpsEphemeris>(current_observable.ephemeris), rtklib_helpers::ConvertSv(current_observable.sv), nav->eph[i]);
				FillObservable(current_observable, gpst2time(nav->eph[i].week, current_observable.time_scale[epoch] * 1e-3), epoch, obs[i]);
			}
			return std::make_pair(std::move(obs), std::move(nav));
		}
	};
}
