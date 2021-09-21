#pragma once

#include "decimator.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../math/ipp_complex_type_converter.hpp"

namespace ugsdr {
	class IppDecimator : public Decimator<IppDecimator> {
	private:
		static auto GetSampleDownWrapper() {
			static auto sample_down_wrapper = plusifier::FunctionWrapper(
				ippsSampleDown_32f, ippsSampleDown_32fc, ippsSampleDown_64f, ippsSampleDown_64fc
			);

			return sample_down_wrapper;
		}


		template <typename TypeToCast, typename T>
		static void ProcessImpl(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			std::vector<T> dst(src_dst.size() / decimation_ratio);
			auto sample_down_wrapper = GetSampleDownWrapper();

			int dst_len = 0;
			int phase = 0*static_cast<int>(decimation_ratio) / 2;
			sample_down_wrapper(reinterpret_cast<const TypeToCast*>(src_dst.data()), static_cast<int>(src_dst.size()), reinterpret_cast<TypeToCast*>(dst.data()),
				&dst_len, static_cast<int>(decimation_ratio), &phase);
			src_dst = std::move(dst);
		}

	protected:
		friend class Decimator<IppDecimator>;

		template <typename UnderlyingType>
		static void Process(std::vector<UnderlyingType>& src_dst, std::size_t decimation_ratio) {
			ProcessImpl<UnderlyingType>(src_dst, decimation_ratio);
		}

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, std::size_t decimation_ratio) {
			using IppType = typename IppComplexTypeConverter<UnderlyingType>::Type;
			ProcessImpl<IppType>(src_dst, decimation_ratio);
		}
	};
}
