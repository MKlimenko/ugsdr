#pragma once

#include "../common.hpp"

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

		template <Container T>
		static auto Calculate(const T& src) {
			return StdDevImpl::Process(src);
		}
	};

	class SequentialMeanStdDev : public MeanStdDev<SequentialMeanStdDev> {
	protected:
		friend class MeanStdDev<SequentialMeanStdDev>;

		template <typename T>
		static auto Process(const std::vector<T>& src) {
			if (src.size() < 2)
				return Result<T>{ 0, 0 };
			auto q = T{};
			auto m = T{};
			for (auto& el : src) {
				q += el * el;
				m += el;
			}
			auto val = (q - m * m / src.size()) / (src.size() - 1);

			return Result<T>{ m / src.size(), std::sqrt(val) };
		}

	public:
	};
}
