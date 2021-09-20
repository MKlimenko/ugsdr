#pragma once

#include "mean_stddev.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	class IppMeanStdDev : public MeanStdDev<IppMeanStdDev> {
	private:
		static auto GetMeanStdDevWrapper() {
			static auto mean_stddev_wrapper = plusifier::FunctionWrapper(
				ippsMeanStdDev_32f, ippsMeanStdDev_64f
			);

			return mean_stddev_wrapper;
		}

	protected:
		friend class MeanStdDev<IppMeanStdDev>;

		template <typename T>
		static auto Process(const std::vector<T>& src_dst) {
			auto stddev_wrapper = GetMeanStdDevWrapper();
			
			T mean{};
			T sigma{};
			stddev_wrapper(src_dst.data(), static_cast<int>(src_dst.size()), &mean, &sigma, IppHintAlgorithm::ippAlgHintNone);
			
			MeanStdDev<IppMeanStdDev>::Result<T> result;
			result.mean = mean;
			result.sigma = sigma;
			return result;
		}
	};
}
