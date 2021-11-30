#pragma once

#include "observable.hpp"
#include "timescale.hpp"
#include "../tracking/tracker.hpp"
#include "../tracking/tracking_parameters.hpp"
#include "../helpers/rtklib_helpers.hpp"

#include <fstream>
#include <memory>
#include <set>
#include <unordered_map>

namespace ugsdr {
	class MeasurementEngine final {
	private:
		void FillObservable(const ugsdr::Observable& current_observable, const gtime_t& time, std::size_t epoch, std::size_t offset, obsd_t& current_rtklib_data) const {
			if (offset == NFREQ + NEXOBS)
				throw std::runtime_error("Number of observables exceeded");
			current_rtklib_data.time = time;
			current_rtklib_data.sat = rtklib_helpers::ConvertSv(current_observable.sv);
			current_rtklib_data.code[offset] = rtklib_helpers::ConvertCode(current_observable.sv);
			current_rtklib_data.P[offset] = current_observable.pseudorange[epoch] / 1000.0 * CLIGHT;
			current_rtklib_data.L[offset] = current_observable.pseudophase[epoch];
			current_rtklib_data.D[offset] = current_observable.doppler[epoch];
			current_rtklib_data.SNR[offset] = current_observable.snr[epoch] / SNR_UNIT;
		}

		void FillEphemeris(const ugsdr::GpsEphemeris& current_ephemeris, std::uint8_t sat, eph_t& current_rtklib_ephemeris) const {
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
	
		void FillEphemeris(const ugsdr::GlonassEphemeris& current_ephemeris, std::uint8_t sat, std::int32_t freq, geph_t& current_rtklib_ephemeris) const {
			current_rtklib_ephemeris.sat = sat;
			current_rtklib_ephemeris.iode = current_ephemeris.tb;
			current_rtklib_ephemeris.frq = freq;
			current_rtklib_ephemeris.svh = current_ephemeris.ln;
			//current_rtklib_ephemeris.sva = 0;		// ??
			//current_rtklib_ephemeris.age = 0;		// ??

			auto adapt_time = [](auto val) {
				val -= 10800;
				if (val < 0)
					val += 86400;
				return val;
			};

			current_rtklib_ephemeris.toe = utc2gpst(gpst2time(week, adapt_time(current_ephemeris.tb)));
			current_rtklib_ephemeris.tof = utc2gpst(gpst2time(week, adapt_time(current_ephemeris.tk)));

			current_rtklib_ephemeris.pos[0] = current_ephemeris.x * 1000;
			current_rtklib_ephemeris.vel[0] = current_ephemeris.x_dot * 1000;
			current_rtklib_ephemeris.acc[0] = current_ephemeris.x_dot_dot * 1000;
			current_rtklib_ephemeris.pos[1] = current_ephemeris.y * 1000;
			current_rtklib_ephemeris.vel[1] = current_ephemeris.y_dot * 1000;
			current_rtklib_ephemeris.acc[1] = current_ephemeris.y_dot_dot * 1000;
			current_rtklib_ephemeris.pos[2] = current_ephemeris.z * 1000;
			current_rtklib_ephemeris.vel[2] = current_ephemeris.z_dot * 1000;
			current_rtklib_ephemeris.acc[2] = current_ephemeris.z_dot_dot * 1000;

			current_rtklib_ephemeris.taun = current_ephemeris.tn;
			current_rtklib_ephemeris.gamn = current_ephemeris.gamma;
			current_rtklib_ephemeris.dtaun = current_ephemeris.delta_tau_n;
		}

		void InitNav(nav_t* ptr) {
			if (!ptr)
				throw std::runtime_error("Unexpected nullptr");

			// revise for multiple signals (L1 C/A and L2C would double the memory)
			auto gps_cnt = std::count_if(observables.begin(), observables.end(), [](auto& obs) {
				return (obs.sv.system != System::Glonass) && (obs.sv.system != System::Sbas);
				});
			ptr->nmax = gps_cnt;
			ptr->eph = new eph_t[gps_cnt];
			
			ptr->ngmax = NSATGLO;
			ptr->geph = new geph_t[NSATGLO];
		}

		static void FreeNav(nav_t* ptr) {
			if (!ptr)
				return;
			if (ptr->eph)
				delete[] ptr->eph;

			if (ptr->geph)
				delete[] ptr->geph;
		}

		void WriteObs(rnxopt_t* rnxopt, std::size_t epoch_step) const {
			if (!rnxopt)
				throw std::runtime_error("Unexpected nullptr");
			rtklib_helpers::FillRinexCodes(rnxopt, available_signals);

			auto rinex_obs = std::unique_ptr<FILE, int(*)(FILE*)>(fopen("ugsdr.obs", "w"), fclose);

			auto status_header = outrnxobsh(rinex_obs.get(), rnxopt, nav.get());

			auto mod = epoch_step == 1 ? 1 : static_cast<std::size_t>(std::fmod(receiver_time_scale.first(), epoch_step));

			for (std::size_t epoch = epoch_step - mod; epoch < receiver_time_scale.length(); epoch += epoch_step) {
				auto [obs, nav] = GetMeasurementEpoch(epoch);
				auto status_body = outrnxobsb(rinex_obs.get(), rnxopt, obs.data(), static_cast<int>(obs.size()), 0);
			}
		}

		void WriteNav(rnxopt_t* rnxopt) const {
			if (!rnxopt)
				throw std::runtime_error("Unexpected nullptr");

			auto rinex_nav = std::unique_ptr<FILE, int(*)(FILE*)>(fopen("ugsdr.nav", "w"), fclose);

			auto status_header = outrnxnavh(rinex_nav.get(), rnxopt, nav.get());
			for (std::size_t i = 0; i < nav->n; ++i)
				outrnxnavb(rinex_nav.get(), rnxopt, nav->eph + i);
			for (std::size_t i = 0; i < nav->ng; ++i)
				outrnxgnavb(rinex_nav.get(), rnxopt, nav->geph + i);
		}

	public:
		TimeScale receiver_time_scale;
		std::vector<Observable> observables;
		std::unique_ptr<nav_t, void(*)(nav_t*)> nav;
		std::set<ugsdr::Signal> available_signals;
		std::size_t week = 0;

		template <typename T>
		MeasurementEngine(const ugsdr::Tracker<T>& tracker) : MeasurementEngine(tracker.GetTrackingParameters()) {}

		template <typename T>
		MeasurementEngine(const std::vector<ugsdr::TrackingParameters<T>>& tracking_results) : nav(new nav_t(), FreeNav) {
			if (tracking_results.empty())
				throw std::runtime_error("Empty tracking results");

			receiver_time_scale = TimeScale(tracking_results.begin()->prompt.size());
			observables.reserve(tracking_results.size());

			for (auto& el : tracking_results) 
				Observable::MakeObservable(el, receiver_time_scale, observables);

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
			for (std::size_t i = 0; i < observables.size(); ++i) {
				available_signals.emplace(observables[i].sv);
				switch (observables[i].sv.system)
				{
				case System::Gps:
					FillEphemeris(std::get<ugsdr::GpsEphemeris>(observables[i].ephemeris), rtklib_helpers::ConvertSv(observables[i].sv), nav->eph[nav->n++]);
					week = std::get<ugsdr::GpsEphemeris>(observables[i].ephemeris).week_number;
					break;
				case System::Glonass: {
					auto fcn_rtklib = observables[i].sv.id + 8;
					observables[i].sv.id = std::get<ugsdr::GlonassEphemeris>(observables[i].ephemeris).n - 1;
					nav->glo_fcn[observables[i].sv.id] = fcn_rtklib;
					FillEphemeris(std::get<ugsdr::GlonassEphemeris>(observables[i].ephemeris), rtklib_helpers::ConvertSv(observables[i].sv), fcn_rtklib, nav->geph[nav->ng++]);
					break;
				}
				default:
					throw std::runtime_error("Unexpected system");
					break;
				}
			}
		}
	
		auto GetMeasurementEpoch(std::size_t epoch) const -> std::pair< std::vector<obsd_t>, nav_t*>{
			auto obs = std::vector<obsd_t>();
			obs.reserve(observables.size());

			auto unique_svs = std::unordered_map<int, std::pair<std::size_t, obsd_t*>>();
			for (std::size_t i = 0; i < observables.size(); ++i) {
				auto& current_sv_cnt = unique_svs[rtklib_helpers::ConvertSv(observables[i].sv)];
				if (current_sv_cnt.first == 0) 
					current_sv_cnt.second = &(obs.emplace_back(obsd_t{ {0} }));

				FillObservable(observables[i], gpst2time(week, receiver_time_scale[epoch] * 1e-3), epoch, current_sv_cnt.first++, *current_sv_cnt.second);
			}
			return std::make_pair(std::move(obs), nav.get());
		}

		void WriteRinex(std::size_t epoch_step = 1) {
			auto rnxopt = std::make_unique<rnxopt_t>();
			rnxopt->rnxver = 303;
			rnxopt->ttol = epoch_step * 0.001;
			rnxopt->tstart = gpst2time(nav->eph[0].week, receiver_time_scale.first() * 1e-3);
			rnxopt->tend = gpst2time(nav->eph[0].week, receiver_time_scale.last() * 1e-3);
			for (auto& el : available_signals)
				rnxopt->navsys |= rtklib_helpers::ConvertSystem(ugsdr::GetSystemBySignal(el));

			rnxopt->obstype = OBSTYPE_ALL;
			rnxopt->freqtype = FREQTYPE_ALL;

			WriteNav(rnxopt.get());
			WriteObs(rnxopt.get(), epoch_step);
		}
	};
}
