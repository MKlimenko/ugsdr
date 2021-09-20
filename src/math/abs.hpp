#pragma once

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename AbsImpl>
	class Abs {
	protected:
	public:
		template <typename UnderlyingType>
		static auto Transform(const std::vector<std::complex<UnderlyingType>>& src) {
			return AbsImpl::Process(src);
		}
	};
}
