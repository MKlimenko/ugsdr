#pragma once

#ifdef HAS_IPP

#include "dft.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../../external/type_map/include/type_map.hpp"

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

		static auto& GetSpec() {
			static thread_local auto spec = plusifier::PointerWrapper<Ipp8u, ippsFree>();
			return spec;
		}

		static auto& GetInitializationBuffer() {
			static thread_local auto init_buf = plusifier::PointerWrapper<Ipp8u, ippsFree>();
			return init_buf;
		}

		static auto& GetWorkBuffer() {
			static thread_local auto work_buf = plusifier::PointerWrapper<Ipp8u, ippsFree>();
			return work_buf;
		}

		template <typename UnderlyingType>
		static void SetDftSizes(std::size_t signal_size) {
			static thread_local std::size_t current_size = 0;

			if (current_size == signal_size)
				return;

			current_size = signal_size;
			Ipp32s spec_size = 0;
			Ipp32s init_size = 0;
			Ipp32s work_size = 0;
			IppDftFunctions<UnderlyingType>::GetSize(static_cast<int>(signal_size), IPP_FFT_DIV_INV_BY_N, 
				ippAlgHintNone, &spec_size, &init_size, &work_size);

			GetSpec() = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, spec_size);
			GetInitializationBuffer() = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, init_size);
			GetWorkBuffer() = plusifier::PointerWrapper<Ipp8u, ippsFree>(ippsMalloc_8u, work_size);

			Ipp8u* spec_ptr = GetSpec();
			using DftSpecType = typename IppDftFunctions<UnderlyingType>::SpecType;
			IppDftFunctions<UnderlyingType>::GetInit(static_cast<int>(signal_size), IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, reinterpret_cast<DftSpecType*>(spec_ptr), GetInitializationBuffer());

		}
		
	protected:
		friend class DiscreteFourierTransform<IppDft>;

		template <typename T>
		struct IppDftFunctions {
			using DftGetSize = mk::TypeMap<
				mk::TypeValuePair<float, ippsDFTGetSize_C_32fc>,
				mk::TypeValuePair<double, ippsDFTGetSize_C_64fc>
			>;
			using DftInit = mk::TypeMap<
				mk::TypeValuePair<float, ippsDFTInit_C_32fc>,
				mk::TypeValuePair<double, ippsDFTInit_C_64fc>
			>;
			using DftInverse = mk::TypeMap<
				mk::TypeValuePair<float, ippsDFTInv_CToC_32fc>,
				mk::TypeValuePair<double, ippsDFTInv_CToC_64fc>
			>;
			using DftForward = mk::TypeMap<
				mk::TypeValuePair<float, ippsDFTFwd_CToC_32fc>,
				mk::TypeValuePair<double, ippsDFTFwd_CToC_64fc>
			>;
			using DftSpecType = mk::TypeMap<
				mk::TypePair<float, IppsDFTSpec_C_32fc>,
				mk::TypePair<double, IppsDFTSpec_C_64fc>
			>;

			template <typename ... Args>
			static auto GetSize(Args &&... args) {
				return DftGetSize::GetValueByType<T>()(args...);
			}
			template <typename ... Args>
			static auto GetInit(Args &&... args) {
				return DftInit::GetValueByType<T>()(args...);
			}
			static auto GetInverse() {
				return DftInverse::GetValueByType<T>();
			}
			static auto GetForward() {
				return DftForward::GetValueByType<T>();
			}
			using SpecType = DftSpecType::GetTypeByType<T>;
		};
		
		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_inverse = false) {
			static_assert(std::is_floating_point_v<UnderlyingType>, R"(Only floating point types are supported, convert integers in the const overload)");
			SetDftSizes<UnderlyingType>(src_dst.size());
			Ipp8u* spec_ptr = GetSpec();
			using DftSpecType = typename IppDftFunctions<UnderlyingType>::SpecType;
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			auto dft_routine = is_inverse ? IppDftFunctions<UnderlyingType>::GetInverse() : IppDftFunctions<UnderlyingType>::GetForward();
			dft_routine(reinterpret_cast<IppType*>(src_dst.data()), reinterpret_cast<IppType*>(src_dst.data()), reinterpret_cast<DftSpecType*>(spec_ptr), GetWorkBuffer());
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

#endif
