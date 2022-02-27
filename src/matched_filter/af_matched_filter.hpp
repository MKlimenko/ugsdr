#pragma once

#ifdef HAS_ARRAYFIRE

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

		template <ComplexContainer T1, Container T2>
		[[nodiscard]]
		static auto Process(const T1& src, const T2& impulse_response) {
			auto src_spectrum = DftImpl::Transform(src);
			auto ir_spectrum = DftImpl::Transform(impulse_response);

			src_spectrum = ArrayProxy(src_spectrum * af::conjg(ir_spectrum));
			DftImpl::Transform(src_spectrum, true);
			return src_spectrum;
		}

		template <ComplexContainer T1, Container T2>
		static void Process(T1& src_dst, const T2& impulse_response) {
			auto src_spectrum = Process(const_cast<const T1&>(src_dst), impulse_response);

			auto matched_output_optional = src_spectrum.CopyFromGpu(src_dst);

			if (matched_output_optional.has_value())
				src_dst = std::move(matched_output_optional.value());
		}

		template <Container T>
		[[nodiscard]]
		static auto Prepare(const T& impulse_response) {
			auto ir_spectrum = DftImpl::Transform(impulse_response);
			return ArrayProxy(af::conjg(ir_spectrum));
		}

		static void ProcessOptimized(ArrayProxy& src_dst, const ArrayProxy& impulse_response) {
			DftImpl::Transform(src_dst);

			src_dst = ArrayProxy(src_dst * static_cast<af::array>(impulse_response));
			DftImpl::Transform(src_dst, true);
		}

		[[nodiscard]]
		static auto ProcessOptimized(const ArrayProxy& src, const ArrayProxy& impulse_response) {
			auto dst = src;
			ProcessOptimized(dst, impulse_response);
			return dst;
		}

		template <ComplexContainer T1, Container T2>
		[[nodiscard]]
		static auto ProcessOptimized(const T1& src, const T2& impulse_response) {
			auto src_spectrum = DftImpl::Transform(src);

			src_spectrum = ArrayProxy(src_spectrum * static_cast<af::array>(impulse_response));
			DftImpl::Transform(src_spectrum, true);
			return src_spectrum;
		}

		template <ComplexContainer T1, Container T2>
		static void ProcessOptimized(T1& src_dst, const T2& impulse_response) {
			auto src_spectrum = ProcessOptimized(src_dst, impulse_response);
			auto matched_output_optional = src_spectrum.CopyFromGpu(src_dst);

			if (matched_output_optional.has_value())
				src_dst = std::move(matched_output_optional.value());
		}
	};

}

#endif
