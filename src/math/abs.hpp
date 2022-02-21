#pragma once

#include "../common.hpp"

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename AbsImpl>
	class Abs {
	protected:
	public:
		template <ComplexContainer T>
		static auto Transform(const T& src) {
			return AbsImpl::Process(src);
		}
	};

	class SequentialAbs : public Abs<SequentialAbs> {
	protected:
		friend class Abs<SequentialAbs>;

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src) {
			auto dst = std::vector<UnderlyingType>(src.size());
			for (std::size_t i = 0; i < src.size(); ++i) {
				dst[i] = std::abs(src[i]);
			}
			return dst;
		}

	public:
	};
}
