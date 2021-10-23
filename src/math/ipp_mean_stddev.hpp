#pragma once

#include "mean_stddev.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	class IppMeanStdDev : public MeanStdDev<IppMeanStdDev> {
	private:
		static auto GetMeanStdDevWrapper() {
			static auto mean_stddev_wrapper = plusifier::FunctionWrapper(
				[](const Ipp32f* src, int length, Ipp32f* mean, Ipp32f* std_dev) { return ippsMeanStdDev_32f(src, length, mean, std_dev, IppHintAlgorithm::ippAlgHintNone); },
				ippsMeanStdDev_64f
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
			stddev_wrapper(src_dst.data(), static_cast<int>(src_dst.size()), &mean, &sigma);
			
			MeanStdDev<IppMeanStdDev>::Result<T> result;
			result.mean = mean;
			result.sigma = sigma;
			return result;
		}
	};
}
