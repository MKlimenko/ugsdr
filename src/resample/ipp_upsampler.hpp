#pragma once

#include "upsampler.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../math/ipp_complex_type_converter.hpp"

namespace ugsdr {
	class IppUpsampler : public Upsampler<IppUpsampler> {
	private:
		static auto GetSampleUpWrapper() {
			static auto sample_up_wrapper = plusifier::FunctionWrapper(
				ippsSampleUp_32f, ippsSampleUp_32fc, ippsSampleUp_64f, ippsSampleUp_64fc
			);

			return sample_up_wrapper;
		}


		template <typename TypeToCast, typename T>
		static void ProcessImpl(std::vector<T>& src_dst, std::size_t samples) {
			std::vector<T> dst(samples);
			auto sample_up_wrapper = GetSampleUpWrapper();

			int dst_len = 0;
			int phase = 0;
			sample_up_wrapper(reinterpret_cast<const TypeToCast*>(src_dst.data()), static_cast<int>(src_dst.size()), reinterpret_cast<TypeToCast*>(dst.data()), &dst_len, static_cast<int>(samples / src_dst.size()), &phase);
			src_dst = std::move(dst);
		}

	protected:
		friend class Upsampler<IppUpsampler>;

		template <typename UnderlyingType>
		static void Process(std::vector<UnderlyingType>& src_dst, std::size_t samples) {
			ProcessImpl<UnderlyingType>(src_dst, samples);
		}

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, std::size_t samples) {
			using IppType = typename IppComplexTypeConverter<UnderlyingType>::Type;
			ProcessImpl<IppType>(src_dst, samples);
		}
	};
}
