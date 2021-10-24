#pragma once

#include "matched_filter.hpp"
#include "ipp.h"
#include "../helpers/ipp_complex_type_converter.hpp"
#include "../math/ipp_dft.hpp"

namespace ugsdr {
	class IppMatchedFilter : public MatchedFilter<IppMatchedFilter> {
	private:
		using DftImpl = ugsdr::IppDft;

		static auto GetMulByConjWrapper() {
			static auto mul_by_conj_wrapper = plusifier::FunctionWrapper(
				ippsMulByConj_32fc_A11, ippsMulByConj_64fc_A26);

			return mul_by_conj_wrapper;
		}
		
		static auto GetConjWrapper() {
			static auto conj_wrapper = plusifier::FunctionWrapper(
				ippsConj_32fc_I, ippsConj_64fc_I
			);

			return conj_wrapper;
		}

		static auto GetMulWrapper() {
			static auto mul_wrapper = plusifier::FunctionWrapper(
				ippsMul_32fc_I, ippsMul_64fc_I
			);

			return mul_wrapper;
		}
		
		template <typename T>
		static void MultiplyByConj(std::vector<std::complex<T>>& signal_spectrum, std::vector<std::complex<T>>& ir_spectrum) {
			using IppType = typename IppTypeToComplex<T>::Type;
			auto conj_wrapper = GetConjWrapper();
			conj_wrapper(reinterpret_cast<IppType*>(ir_spectrum.data()), static_cast<int>(ir_spectrum.size()));
			auto mul_wrapper = GetMulWrapper();
			mul_wrapper(reinterpret_cast<const IppType*>(ir_spectrum.data()),
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

		template <typename T >
		static auto Prepare(const std::vector<T>& impulse_response) {
			auto ir_spectrum = DftImpl::Transform(impulse_response);
			auto conj_wrapper = GetConjWrapper();
			using IppType = typename IppTypeToComplex<underlying_t<T>>::Type;
			conj_wrapper(reinterpret_cast<IppType*>(ir_spectrum.data()), static_cast<int>(ir_spectrum.size()));
			return ir_spectrum;
		}

		template <typename UnderlyingType, typename T>
		static void ProcessOptimized(std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			DftImpl::Transform(src_dst);

			auto mul_wrapper = GetMulWrapper();
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			mul_wrapper(reinterpret_cast<const IppType*>(impulse_response.data()), reinterpret_cast<IppType*>(src_dst.data()), static_cast<int>(src_dst.size()));

			DftImpl::Transform(src_dst, true);
		}

		template <typename UnderlyingType, typename T>
		static auto ProcessOptimized(const std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			const auto dst = src_dst;
			ProcessOptimized(dst, impulse_response);
			return dst;
		}
	};

}
