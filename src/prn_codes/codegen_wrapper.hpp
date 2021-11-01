#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "GalileoE1b.hpp"
#include "GalileoE1c.hpp"
#include "GalileoE5aI.hpp"
#include "GalileoE5aQ.hpp"
#include "GpsL1Ca.hpp"
#include "GlonassOf.hpp"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	using CodegenImplMap = mk::TypeMap<
		mk::ValueTypePair<Signal::GpsCoarseAcquisition_L1, GpsL1Ca>,
		mk::ValueTypePair<Signal::GlonassCivilFdma_L1, GlonassOf>,
		mk::ValueTypePair<Signal::Galileo_E1b, GalileoE1b>,
		mk::ValueTypePair<Signal::Galileo_E1c, GalileoE1c>,
		mk::ValueTypePair<Signal::Galileo_E5aI, GalileoE5aI>,
		mk::ValueTypePair<Signal::Galileo_E5aQ, GalileoE5aQ>

	>;
	
	template <Signal sig>
	using PrnGenerator = Codegen<CodegenImplMap::GetTypeByValue<sig>>;
}
