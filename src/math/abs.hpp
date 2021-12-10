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
