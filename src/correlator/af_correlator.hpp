#pragma once

#include "arrayfire.h"
#include "correlator.hpp"
#include "../helpers/af_array_proxy.hpp"

namespace ugsdr {
	class AfCorrelator : public Correlator<AfCorrelator> {
	protected:
		friend class Correlator<AfCorrelator>;

		static auto Process(const ArrayProxy& signal, const ArrayProxy& code) {
			if (signal.size() != code.size())
				throw std::runtime_error("Size mismatch");

			auto dot_product = ArrayProxy(af::dot(signal, code));

			std::complex<float> dst{};
			auto dst_vec = static_cast<std::vector<std::complex<float>>>(dot_product);
			dst = dst_vec.at(0);
			
			return dst;
		}
	};
}