#pragma once

#include "ipp.h"
#include "correlator.hpp"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

namespace ugsdr {
	class IppCorrelator : public Correlator<IppCorrelator> {
	private:
		static auto GetDotWrapper() {
			static auto dot_wrapper = plusifier::FunctionWrapper(
				ippsDotProd_32f32fc, ippsDotProd_32fc, ippsDotProd_64f64fc, ippsDotProd_64fc
			);

			return dot_wrapper;
		}

	protected:
		friend class Correlator<IppCorrelator>;

		template <typename UnderlyingType, typename T>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& signal, const std::span<T>& code) {
			if (signal.size() != code.size())
				throw std::runtime_error("Size mismatch");

			auto dot_wrapper = GetDotWrapper();

			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			std::complex<UnderlyingType> dst{};
			dot_wrapper(code.data(), reinterpret_cast<const IppType*>(signal.data()), static_cast<int>(signal.size()), reinterpret_cast<IppType*>(&dst));

			return dst;
		}
	};
}