#pragma once

#include <complex>
#include <vector>

#include "../math/conj.hpp"
#include "../math/dft.hpp"

namespace ugsdr {
	template <typename FilterImpl>
	class MatchedFilter {
	protected:
	public:
		template <ComplexContainer T1, Container T2>
		static void Filter(T1& src_dst, const T2& impulse_response) {
			FilterImpl::Process(src_dst, impulse_response);
		}

		template <ComplexContainer T1, Container T2>
		static auto Filter(const T1& src_dst, const T2& impulse_response) {
			auto dst = src_dst;
			FilterImpl::Process(dst, impulse_response);
			return dst;
		}

		template <Container T>
		static auto PrepareCodeSpectrum(const T& impulse_response) {
			return FilterImpl::Prepare(impulse_response);
		}

		template <ComplexContainer T1, Container T2>
		static void FilterOptimized(T1& src_dst, const T2& impulse_response) {
			FilterImpl::ProcessOptimized(src_dst, impulse_response);
		}

		template <ComplexContainer T1, Container T2>
		static auto FilterOptimized(const T1& src_dst, const T2& impulse_response) {
			auto dst = src_dst;
			FilterImpl::ProcessOptimized(dst, impulse_response);
			return dst;
		}
	};

	class SequentialMatchedFilter : public MatchedFilter<SequentialMatchedFilter> {
	protected:
		friend class MatchedFilter<SequentialMatchedFilter>;

		template <Container T>
		static auto Prepare(const std::vector<T>& impulse_response) {
			auto ir_spectrum = SequentialDft::Transform(impulse_response);
			SequentialConj::Transform(ir_spectrum);
			return ir_spectrum;
		}

		template <ComplexContainer T1, Container T2>
		static auto ProcessOptimized(T1& src_dst, const T2& impulse_response) {
			SequentialDft::Transform(src_dst);
			std::transform(src_dst.begin(), src_dst.end(), impulse_response.begin(), src_dst.begin(), std::multiplies<typename T1::value_type>{});
			SequentialDft::Transform(src_dst, true);
		}

		template <ComplexContainer T1, Container T2>
		static void Process(T1& src_dst, const T2& impulse_response) {
			auto ir_spectrum = SequentialDft::Transform(impulse_response);
			SequentialConj::Transform(ir_spectrum);
			ProcessOptimized(src_dst, ir_spectrum);
		}
	};
}
