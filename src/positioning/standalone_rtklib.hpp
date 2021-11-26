#pragma once

#include "../helpers/rtklib_helpers.hpp"
#include "standalone_engine.hpp"

#include "../../external/plusifier/Plusifier.hpp"

namespace ugsdr {
	class StandaloneRtklib final : public StandaloneEngine<StandaloneRtklib> {
	protected:
		friend class StandaloneEngine<StandaloneRtklib>;
		prcopt_t processing_options = prcopt_default;

		auto Estimate(std::size_t epoch) {
			auto [obs, nav] = measurement_engine.GetMeasurementEpoch(epoch);
			std::vector<double> azel(obs.size() * 2);
			std::vector<ssat_t> ssat(MAXSAT);
			auto sol = std::make_unique<sol_t>();
			char msg[128] = "";
			auto status = pntpos(obs.data(), obs.size(), nav, &processing_options, sol.get(), azel.data(), ssat.data(), msg);

			if (!status)
				std::cout << "No solution: " << msg << std::endl;
			else {
				std::cout << "Valid solution:" << std::endl;
				std::cout << "\tNumber of satellites: " << static_cast<int>(sol->ns) << std::endl;
				std::cout << "\tCoordinates and time: ";
				for (std::size_t j = 0; j < 3; ++j)
					std::cout << std::fixed << sol->rr[j] << " ";
				std::cout << std::fixed << sol->dtr[0] << std::endl;
			}


			return 0;
		}

	public:
		StandaloneRtklib(MeasurementEngine& measurements) : StandaloneEngine<StandaloneRtklib>(measurements) {
			for (auto& el : measurement_engine.observables)
				processing_options.navsys |= rtklib_helpers::ConvertSystem(el.sv);
		}
	};
}