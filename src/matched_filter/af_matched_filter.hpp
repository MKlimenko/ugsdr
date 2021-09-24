#pragma once

#include "matched_filter.hpp"
#include "arrayfire.h"
#include "../helpers/ipp_complex_type_converter.hpp"
#include "../math/af_dft.hpp"

namespace ugsdr {
	class AfMatchedFilter : public MatchedFilter<AfMatchedFilter> {
	private:
		using DftImpl = ugsdr::AfDft;

	protected:
		friend class MatchedFilter<AfMatchedFilter>;

		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			auto src_spectrum = DftImpl::Transform(const_cast<const std::vector<std::complex<UnderlyingType>>&>(src_dst));
			auto ir_spectrum = DftImpl::Transform(impulse_response);

			src_spectrum = ArrayProxy(src_spectrum * af::conjg(ir_spectrum));
			DftImpl::Transform(src_spectrum, true);

			auto matched_output_optional = src_spectrum.CopyFromGpu(src_dst);

			if (matched_output_optional.has_value())
				src_dst = std::move(matched_output_optional.value());
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
			return ArrayProxy(af::conjg(ir_spectrum));
		}

		template <typename UnderlyingType, typename T>
		static void ProcessOptimized(std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			auto src_spectrum = DftImpl::Transform(const_cast<const std::vector<std::complex<UnderlyingType>>&>(src_dst));

			src_spectrum = ArrayProxy(src_spectrum * static_cast<af::array>(impulse_response));
			DftImpl::Transform(src_spectrum, true);

			auto matched_output_optional = src_spectrum.CopyFromGpu(src_dst);

			if (matched_output_optional.has_value())
				src_dst = std::move(matched_output_optional.value());
		}

		template <typename UnderlyingType, typename T>
		static auto ProcessOptimized(const std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			const auto dst = src_dst;
			ProcessOptimized(dst, impulse_response);
			return dst;
		}
	};

}
