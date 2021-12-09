#pragma once

#ifdef HAS_IPP

#include "decimator.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

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

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			ProcessImpl<T>(src_dst, decimation_ratio);
		}

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, std::size_t decimation_ratio) {
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			ProcessImpl<IppType>(src_dst, decimation_ratio);
		}
	};

	class IppAccumulator : public Decimator<IppAccumulator> {
	private:
		static auto GetSumWrapper() {
			static auto sum_wrapper = plusifier::FunctionWrapper(
				[](const Ipp32f* src, int len, Ipp32f* sum) { return ippsSum_32f(src, len, sum, IppHintAlgorithm::ippAlgHintNone); },
				[](const Ipp32fc* src, int len, Ipp32fc* sum) { return ippsSum_32fc(src, len, sum, IppHintAlgorithm::ippAlgHintNone); },
				ippsSum_64f, 
				ippsSum_64fc
			);
			
			return sum_wrapper;
		}

		static auto GetDivWrapper() {
			static auto div_wrapper = plusifier::FunctionWrapper(
				ippsDivC_32f_I, ippsDivC_32fc_I, ippsDivC_64f_I, ippsDivC_64fc_I
			);

			return div_wrapper;
		}

		template <typename TypeToCast, typename UnderlyingType, typename T>
		static void ProcessImpl(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			if (decimation_ratio == 1)
				return;
			auto [rem, quot] = std::div(src_dst.size(), static_cast<std::int64_t>(decimation_ratio));

			if (quot)
				src_dst.resize((rem + 1) * decimation_ratio);
			
			auto sum_wrapper = GetSumWrapper();
			for (std::size_t i = 0, k = 0; i < src_dst.size(); i += decimation_ratio) 
				sum_wrapper(reinterpret_cast<TypeToCast*>(src_dst.data()) + i, static_cast<int>(decimation_ratio),
					reinterpret_cast<TypeToCast*>(src_dst.data()) + k++);
			
			src_dst.resize(src_dst.size() / decimation_ratio);
			
			auto div_wrapper = GetDivWrapper();
			div_wrapper(TypeToCast{ static_cast<UnderlyingType>(decimation_ratio) }, reinterpret_cast<TypeToCast*>(src_dst.data()), static_cast<int>(src_dst.size()));
		}

	protected:
		friend class Decimator<IppAccumulator>;


		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>&src_dst, std::size_t decimation_ratio) {
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			ProcessImpl<IppType, UnderlyingType>(src_dst, decimation_ratio);
		}

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t decimation_ratio) {
			ProcessImpl<T, T>(src_dst, decimation_ratio);
		}

	public:
	};
}

#endif
