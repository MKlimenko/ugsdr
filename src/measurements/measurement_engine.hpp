#pragma once

#include "observable.hpp"
#include "timescale.hpp"
#include "../tracking/tracker.hpp"
#include "../tracking/tracking_parameters.hpp"

namespace ugsdr {
	class MeasurementEngine final {
		TimeScale receiver_time_scale;
		std::vector<Observable> observables;

	public:
		template <typename T>
		MeasurementEngine(const ugsdr::Tracker<T>& tracker) : MeasurementEngine(tracker.GetTrackingParameters()) {}

		template <typename T>
		MeasurementEngine(const std::vector<ugsdr::TrackingParameters<T>>& tracking_results) {
			if (tracking_results.empty())
				throw std::runtime_error("Empty tracking results");
			
			receiver_time_scale = TimeScale(tracking_results.begin()->prompt.size());
			observables.reserve(tracking_results.size());

			for (auto& el : tracking_results) {
				auto current_observable = Observable::MakeObservable(el, receiver_time_scale);
				if (current_observable)
					observables.push_back(current_observable.value());
			}
		}
	};
}
