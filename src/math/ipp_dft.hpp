#pragma once

#include "dft.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"

#include <type_traits>

namespace ugsdr {
	class IppDft : public DiscreteFourierTransform<IppDft> {
	private:
		static auto GetConversionWrapper() {
			static auto conversion_wrapper = plusifier::FunctionWrapper(
				ippsConvert_8u32f, ippsConvert_8s32f,
				ippsConvert_16u32f, ippsConvert_16s32f,
				ippsConvert_32s32f); // if required, add lambdas for 32u32f, 64s32f etc

			return conversion_wrapper;
		}

	protected:
		friend class DiscreteFourierTransform<IppDft>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			static_assert(std::is_same_v<UnderlyingType, float>, R"(I'll start with float, more types will be added later)");

			Ipp32s spec_size = 0;
			Ipp32s init_size = 0;
			Ipp32s work_size = 0;
			ippsDFTGetSize_C_32fc(static_cast<int>(src_dst.size()), IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &spec_size, &init_size, &work_size);

			auto spec = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, spec_size);
			auto init_buf = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, init_size);
			auto work_buf = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, work_size);

			auto dft_routine = is_inverse ? ippsDFTInv_CToC_32fc : ippsDFTFwd_CToC_32fc;

			Ipp8u* spec_ptr = spec;
			ippsDFTInit_C_32fc(static_cast<int>(src_dst.size()), IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, reinterpret_cast<IppsDFTSpec_C_32fc*>(spec_ptr), init_buf);
			dft_routine(reinterpret_cast<Ipp32fc*>(src_dst.data()), reinterpret_cast<Ipp32fc*>(src_dst.data()), reinterpret_cast<IppsDFTSpec_C_32fc*>(spec_ptr), work_buf);
		}

		template <typename T>
		static auto Process(const std::vector<T>& src, bool is_inverse = false) {
			std::vector<std::complex<T>> dst(src.begin(), src.end());
			Process(dst, is_inverse);
			return dst;
		}
		
		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src, bool is_inverse = false) {
			auto dst = src;
			Process(dst, is_inverse);
			return dst;
		}
	};
}
