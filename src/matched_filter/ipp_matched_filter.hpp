#pragma once

#include "matched_filter.hpp"

namespace ugsdr {
	class IppMatchedFilter : public MatchedFilter<IppMatchedFilter> {
	protected:
		friend class MatchedFilter<IppMatchedFilter>;
		
		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
		}

		template <typename UnderlyingType, typename T>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			auto dst = src_dst;
			return dst;
		}
	};

}
