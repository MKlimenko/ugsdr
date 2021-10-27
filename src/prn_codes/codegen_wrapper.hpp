#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "GalileoE1b.hpp"
#include "GpsL1Ca.hpp"
#include "GlonassOf.hpp"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	using CodegenImplMap = mk::TypeMap<
		mk::ValueTypePair<System::Gps, GpsL1Ca>,
		mk::ValueTypePair<System::Glonass, GlonassOf>,
		mk::ValueTypePair<System::Galileo, GalileoE1b>
	>;
	
	template <System sys>
	using PrnGenerator = Codegen<CodegenImplMap::GetTypeByValue<sys>>;
}
