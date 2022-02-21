#pragma once

#include "mean_stddev.hpp"

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "../helpers/af_array_proxy.hpp"
#include <type_traits>

namespace ugsdr {
	class AfMeanStdDev : public MeanStdDev<AfMeanStdDev> {
	protected:
		friend class MeanStdDev<AfMeanStdDev>;
		
		static auto Process(const ArrayProxy& src_dst) {
			auto mean = af::mean<double>(src_dst);
			auto stddev = af::stdev<double>(src_dst);
			auto result = Result<double>{ mean, stddev };
			return result;
		}
	};
}

#endif
