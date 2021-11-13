#pragma once

#include "../measurements/measurement_engine.hpp"

namespace ugsdr {
	template <typename StandaloneImpl>
	class StandaloneEngine {
	protected:
		MeasurementEngine& measurement_engine;

	public:
		StandaloneEngine(ugsdr::MeasurementEngine& measurements) : measurement_engine(measurements) {}

		auto EstimatePosition(std::size_t epoch) {
			return static_cast<StandaloneImpl*>(this)->Estimate(epoch);
		}
	};
}
