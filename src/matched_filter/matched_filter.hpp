#pragma once

#include <complex>
#include <vector>

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
	};

	class SequentialMatchedFilter : public MatchedFilter<SequentialMatchedFilter> {
	protected:
		friend class MatchedFilter<SequentialMatchedFilter>;

		template <typename UnderlyingType, typename T>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, const std::vector<T>& impulse_response) {
			auto dst = src_dst;
			for (std::size_t i = 0; i < src_dst.size(); ++i) {
				dst[i] = 0;
				for (std::size_t j = 0; j < impulse_response.size(); j++)
					dst[i] += impulse_response[j % impulse_response.size()] * src_dst[(i + j) % src_dst.size()];
			}

			src_dst = dst;
		}

	public:
	};
}
