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
		
		static auto GetFillWrapper() {
			static auto fill_wrapper = plusifier::FunctionWrapper(
				ippsSet_32f, ippsSet_32fc, ippsSet_64f, ippsSet_64fc
			);

			return fill_wrapper;
		}


		template <typename TypeToCast, typename T>
		static void ProcessImpl(std::vector<T>& src_dst, std::size_t samples) {
			std::vector<T> dst(samples);

			auto fill_wrapper = GetFillWrapper();
			auto ratio = samples / src_dst.size();
			for (std::size_t i = 0; i < src_dst.size(); ++i)
				fill_wrapper(reinterpret_cast<TypeToCast*>(src_dst.data())[i], reinterpret_cast<TypeToCast*>(dst.data()) + i * ratio, static_cast<int>(ratio));
		
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
