#pragma once

#include "matched_filter.hpp"
#include "ipp.h"
#include "../math/ipp_dft.hpp"
#include "../math/ipp_conj.hpp"

namespace ugsdr {
	class IppMatchedFilter : public MatchedFilter<IppMatchedFilter> {
	private:

		using DftImpl = ugsdr::IppDft;
		using ConjImpl = ugsdr::IppConj;
				
	protected:
		friend class MatchedFilter<IppMatchedFilter>;
		
		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			DftImpl::Transform(src_dst);
			auto ir_spectrum = DftImpl::Transform(impulse_response);
			ConjImpl::Transform(ir_spectrum);
		}

		template <typename UnderlyingType, typename T>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			const auto dst = src_dst;
			Process(src_dst, impulse_response);
			return dst;
		}
	};

}
