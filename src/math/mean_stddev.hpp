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

	class SequentialMeanStdDev : public MeanStdDev<SequentialMeanStdDev> {
	protected:
		friend class MeanStdDev<SequentialMeanStdDev>;

		template <typename T>
		static auto Process(const std::vector<T>& src) {
			return Result{};
		}

	public:
	};
}
