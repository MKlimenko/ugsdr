#pragma once

#include "../common.hpp"

#include "rtklib.h"

#include <stdexcept>

namespace ugsdr::rtklib_helpers {
	auto ConvertSv(Sv sv) {
		auto sys = SYS_GPS;
		switch (sv.system) {
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

		auto prn = sv.id + 1;

		auto dst = satno(sys, prn);
		if (dst == 0)
			throw std::runtime_error("Unexpected satellite id value");

		return dst;
	}

	auto ConvertCode(Sv sv) {
		switch (sv.signal) {
		case Signal::GpsCoarseAcquisition_L1:
			return CODE_L1C;
		default:
			throw std::runtime_error("Unexpected system");
		}
	}
}
