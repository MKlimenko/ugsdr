#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../acquisition/acquisition_result.hpp"

#include <map>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class Tracker final {
		struct Codes final {
			
		};

		const SignalParametersBase<UnderlyingType>& signal_parameters;
		const std::vector<AcquisitionResult>& acquisition_results;

		Codes codes;
		
	public:
		Tracker(const SignalParametersBase<UnderlyingType>& signal_params, 
			const std::vector<AcquisitionResult>& acquisition_dst) : signal_parameters(signal_params), acquisition_results(acquisition_dst) {
			
		}
	};
}
