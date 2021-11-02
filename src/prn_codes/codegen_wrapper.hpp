#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "GalileoE1b.hpp"
#include "GalileoE1c.hpp"
#include "GalileoE5aI.hpp"
#include "GalileoE5aQ.hpp"
#include "GalileoE5bI.hpp"
#include "GalileoE5bQ.hpp"
#include "GpsL1Ca.hpp"
#include "GpsL2CM.hpp"
#include "GlonassOf.hpp"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	using CodegenImplMap = mk::TypeMap<
		mk::ValueTypePair<Signal::GpsCoarseAcquisition_L1, GpsL1Ca>,
		mk::ValueTypePair<Signal::Gps_L2CM, GpsL2CM>,
		mk::ValueTypePair<Signal::GlonassCivilFdma_L1, GlonassOf>,
		mk::ValueTypePair<Signal::GlonassCivilFdma_L2, GlonassOf>,
		mk::ValueTypePair<Signal::Galileo_E1b, GalileoE1b>,
		mk::ValueTypePair<Signal::Galileo_E1c, GalileoE1c>,
		mk::ValueTypePair<Signal::Galileo_E5aI, GalileoE5aI>,
		mk::ValueTypePair<Signal::Galileo_E5aQ, GalileoE5aQ>,
		mk::ValueTypePair<Signal::Galileo_E5bI, GalileoE5bI>,
		mk::ValueTypePair<Signal::Galileo_E5bQ, GalileoE5bQ>
	>;
	
	template <Signal sig>
	using PrnGenerator = Codegen<CodegenImplMap::GetTypeByValue<sig>>;
}
