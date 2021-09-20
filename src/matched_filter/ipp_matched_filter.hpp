#pragma once

#include "matched_filter.hpp"
#include "ipp.h"
#include "../math/ipp_dft.hpp"
#include "../math/ipp_complex_type_converter.hpp"

namespace ugsdr {
	class IppMatchedFilter : public MatchedFilter<IppMatchedFilter> {
	private:
		using DftImpl = ugsdr::IppDft;

		static auto GetMulByConjWrapper() {
			static auto mul_by_conj_wrapper = plusifier::FunctionWrapper(
				ippsMulByConj_32fc_A11, ippsMulByConj_64fc_A26);

			return mul_by_conj_wrapper;
		}
		
		template <typename T>
		static void MultiplyByConj(std::vector<std::complex<T>>& signal_spectrum, const std::vector<std::complex<T>>& ir_spectrum) {
			using IppType = typename IppComplexTypeConverter<T>::Type;
			auto mul_by_conj_wrapper = GetMulByConjWrapper();
			mul_by_conj_wrapper(reinterpret_cast<const IppType*>(signal_spectrum.data()), reinterpret_cast<const IppType*>(ir_spectrum.data()),
				reinterpret_cast<IppType*>(signal_spectrum.data()), static_cast<int>(signal_spectrum.size()));
		}
		
	protected:
		friend class MatchedFilter<IppMatchedFilter>;
		
		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			DftImpl::Transform(src_dst);
			auto ir_spectrum = DftImpl::Transform(impulse_response);
			MultiplyByConj(src_dst, ir_spectrum);
			DftImpl::Transform(src_dst, true);
		}

		template <typename UnderlyingType, typename T>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			const auto dst = src_dst;
			Process(src_dst, impulse_response);
			return dst;
		}
	};

}
