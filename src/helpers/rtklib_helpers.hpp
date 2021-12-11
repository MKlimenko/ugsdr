#pragma once

#include "../common.hpp"

#include "rtklib.h"

#include <set>
#include <stdexcept>

namespace ugsdr::rtklib_helpers {
	inline auto ConvertSystem(System system) {
		auto sys = SYS_GPS;
		switch (system) {
		case System::Gps:
			sys = SYS_GPS;
			break;
		case System::Glonass:
			sys = SYS_GLO;
			break;
		case System::Galileo:
			sys = SYS_GAL;
			break;
		case System::BeiDou:
			sys = SYS_CMP;
			break;
		case System::NavIC:
			sys = SYS_IRN;
			break;
		case System::Sbas:
			sys = SYS_SBS;
			break;
		case System::Qzss:
			sys = SYS_QZS;
			break;
		default:
			throw std::runtime_error("Unexpected system");
			break;
		}
		return sys;
	}

	inline auto ConvertSystemIndex(System system) {
		// GPS,GLO,GAL,QZS,SBS,CMP,IRN
		switch (system) {
		case System::Gps:
			return 0;
		case System::Glonass:
			return 1;
		case System::Galileo:
			return 2;
		case System::BeiDou:
			return 5;
		case System::NavIC:
			return 6;
		case System::Sbas:
			return 4;
		case System::Qzss:
			return 3;
		default:
			throw std::runtime_error("Unexpected system");
		}
	}


	inline auto ConvertSv(Sv sv) {
		auto sys = ConvertSystem(sv);

		auto prn = sv.id + 1;

		auto dst = satno(sys, prn);
		if (dst == 0)
			throw std::runtime_error("Unexpected satellite id value");

		return static_cast<std::uint8_t>(dst);
	}

	inline std::uint8_t ConvertCode(Sv sv) {
		switch (sv.signal) {
		case Signal::GpsCoarseAcquisition_L1:
		case Signal::GlonassCivilFdma_L1:
			return CODE_L1C;
		case Signal::Gps_L2CM:
			return CODE_L2S;
		case Signal::GlonassCivilFdma_L2:
			return CODE_L2C;
		default:
			throw std::runtime_error("Unexpected signal");
		}
	}

	inline std::string GetRinexSuffix(Signal signal) {
		switch (signal) {
		case Signal::GpsCoarseAcquisition_L1:
		case Signal::GlonassCivilFdma_L1:
			return "1C";
		case Signal::Gps_L2CM:
			return "2S";
		case Signal::GlonassCivilFdma_L2:
			return "2C";
		default:
			throw std::runtime_error("Unexpected system");
		}
	}

	inline void FillRinexCodes(rnxopt_t* rnxopt, const std::set<Signal>& available_signals) {
		if (!rnxopt)
			throw std::runtime_error("Unexpected nullptr");

		auto obs_types_prefix = std::array<char, 4> { 'C', 'L', 'D', 'S' };

		for (const auto& signal : available_signals) {
			auto rtklib_system = ConvertSystemIndex(ugsdr::GetSystemBySignal(signal));
			auto offset = rnxopt->nobs[rtklib_system];
			rnxopt->nobs[rtklib_system] += 4;
			
			auto suffix = GetRinexSuffix(signal);
			for (std::size_t i = offset; i < offset + 4; ++i) {
				auto current_string = obs_types_prefix[i - offset] + suffix;
				std::copy(current_string.begin(), current_string.end(), rnxopt->tobs[rtklib_system][i]);
			}
		}
	}
}
