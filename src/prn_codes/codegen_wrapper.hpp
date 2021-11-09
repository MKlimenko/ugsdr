#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "BeiDouB1I.hpp"
#include "BeiDouB1C.hpp"
#include "GalileoE1b.hpp"
#include "GalileoE1c.hpp"
#include "GalileoE5aI.hpp"
#include "GalileoE5aQ.hpp"
#include "GalileoE5bI.hpp"
#include "GalileoE5bQ.hpp"
#include "GlonassOf.hpp"
#include "GpsL1Ca.hpp"
#include "GpsL2CM.hpp"
#include "GpsL5I.hpp"
#include "GpsL5Q.hpp"
#include "NavICL5Ca.hpp"
#include "SbasL1Ca.hpp"
#include "SbasL5I.hpp"
#include "SbasL5Q.hpp"
#include "QzssL1Ca.hpp"
#include "QzssL1Saif.hpp"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	using CodegenImplMap = mk::TypeMap<
		mk::ValueTypePair<Signal::GpsCoarseAcquisition_L1, GpsL1Ca>,
		mk::ValueTypePair<Signal::Gps_L2CM, GpsL2CM>,
		mk::ValueTypePair<Signal::Gps_L5I, GpsL5I>,
		mk::ValueTypePair<Signal::Gps_L5Q, GpsL5Q>,
		mk::ValueTypePair<Signal::GlonassCivilFdma_L1, GlonassOf>,
		mk::ValueTypePair<Signal::GlonassCivilFdma_L2, GlonassOf>,
		mk::ValueTypePair<Signal::Galileo_E1b, GalileoE1b>,
		mk::ValueTypePair<Signal::Galileo_E1c, GalileoE1c>,
		mk::ValueTypePair<Signal::Galileo_E5aI, GalileoE5aI>,
		mk::ValueTypePair<Signal::Galileo_E5aQ, GalileoE5aQ>,
		mk::ValueTypePair<Signal::Galileo_E5bI, GalileoE5bI>,
		mk::ValueTypePair<Signal::Galileo_E5bQ, GalileoE5bQ>,
		mk::ValueTypePair<Signal::BeiDou_B1I, BeiDouB1I>,
		mk::ValueTypePair<Signal::BeiDou_B1C, BeiDouB1C>,
		mk::ValueTypePair<Signal::NavIC_L5, NavICL5Ca>,
		mk::ValueTypePair<Signal::SbasCoarseAcquisition_L1, SbasL1Ca>,
		mk::ValueTypePair<Signal::Sbas_L5I, SbasL5I>,
		mk::ValueTypePair<Signal::Sbas_L5Q, SbasL5Q>,
		mk::ValueTypePair<Signal::QzssCoarseAcquisition_L1, QzssL1Ca>,
		mk::ValueTypePair<Signal::Qzss_L1S, QzssL1Saif>
	>;
	
	template <Signal sig>
	using PrnGenerator = Codegen<CodegenImplMap::GetTypeByValue<sig>>;
}
