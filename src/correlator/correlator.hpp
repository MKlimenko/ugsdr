#pragma once

#include <complex>
#include <numeric>
#include <span>
#include <vector>

namespace ugsdr {
	template <typename CorrelatorImpl>
	class Correlator {
	protected:
	public:

		template <typename T1, typename T2>
		static auto Correlate(const T1& signal, const T2& code) {
			return CorrelatorImpl::Process(signal, code);
		}
	};

	class SequentialCorrelator : public Correlator<SequentialCorrelator> {
	protected:
		friend class Correlator<SequentialCorrelator>;

		template <typename UnderlyingType, typename T>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& signal, const std::span<T>& code) {
			if (signal.size() != code.size())
				throw std::runtime_error("Size mismatch");

			return std::inner_product(signal.begin(), signal.end(), code.begin(), std::complex<UnderlyingType>{});
		}
	};
}
