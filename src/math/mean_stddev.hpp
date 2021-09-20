#pragma once

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename StdDevImpl>
	class MeanStdDev {
	protected:
	public:
		template <typename T>
		struct Result {
			T mean{};
			T sigma{};
		};

		template <typename T>
		static auto Calculate(const std::vector<T>& src) {
			return StdDevImpl::Process(src);
		}
	};
}
