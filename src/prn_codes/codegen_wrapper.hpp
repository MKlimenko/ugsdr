#pragma once

#include "../common.hpp"
#include "codegen.hpp"
#include "GpsL1Ca.hpp"
#include "GlonassOf.hpp"
#include "../../external/type_map/include/type_map.hpp"

namespace ugsdr {
	using CodegenImplMap = mk::TypeMap<
		mk::ValueTypePair<System::Gps, GpsL1Ca>,
		mk::ValueTypePair<System::Glonass, GlonassOf>
	>;
	
	template <System sys>
	using PrnGenerator = Codegen<CodegenImplMap::GetTypeByValue<sys>>;
}
