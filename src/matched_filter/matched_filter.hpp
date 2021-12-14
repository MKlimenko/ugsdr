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
		template <typename UnderlyingType, typename T>
		static void Filter(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			FilterImpl::Process(src_dst, impulse_response);
		}

		template <typename UnderlyingType, typename T>
		static auto Filter(const std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			auto dst = src_dst;
			FilterImpl::Process(dst, impulse_response);
			return dst;
		}

		template <typename T>
		static auto PrepareCodeSpectrum(const std::vector<T>& impulse_response) {
			return FilterImpl::Prepare(impulse_response);
		}

		template <typename UnderlyingType, typename T>
		static void FilterOptimized(std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			FilterImpl::ProcessOptimized(src_dst, impulse_response);
		}

		template <typename UnderlyingType, typename T>
		static auto FilterOptimized(const std::vector<std::complex<UnderlyingType>>& src_dst, const T& impulse_response) {
			auto dst = src_dst;
			FilterImpl::ProcessOptimized(dst, impulse_response);
			return dst;
		}
	};

	class SequentialMatchedFilter : public MatchedFilter<SequentialMatchedFilter> {
	protected:
		friend class MatchedFilter<SequentialMatchedFilter>;

		template <typename T>
		static auto Prepare(const std::vector<T>& impulse_response) {
			auto ir_spectrum = SequentialDft::Transform(impulse_response);
			SequentialConj::Transform(ir_spectrum);
			return ir_spectrum;
		}

		template <typename UnderlyingType, typename T>
		static auto ProcessOptimized(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			SequentialDft::Transform(src_dst);
			std::transform(src_dst.begin(), src_dst.end(), impulse_response.begin(), src_dst.begin(), std::multiplies<std::complex<UnderlyingType>>{});
			SequentialDft::Transform(src_dst, true);
		}

		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			auto ir_spectrum = SequentialDft::Transform(impulse_response);
			SequentialConj::Transform(ir_spectrum);
			ProcessOptimized(src_dst, ir_spectrum);
		}

	public:
	};
}
