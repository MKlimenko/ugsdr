#pragma once

#include "conj.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	class IppConj : public ComplexConjugate<IppConj> {
	private:
		static auto GetConjWrapper() {
			static auto conj_wrapper = plusifier::FunctionWrapper(
				ippsConj_16sc, ippsConj_32fc, ippsConj_64fc_A53);

			return conj_wrapper;
		}
		
	protected:
		friend class ComplexConjugate<IppConj>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst) {
			auto conj_wrapper = GetConjWrapper();
			using IppType = IppComplexTypeConverter<UnderlyingType>::Type;
						
			conj_wrapper(reinterpret_cast<const IppType*>(src_dst.data()), reinterpret_cast<IppType*>(src_dst.data()), static_cast<int>(src_dst.size()));
		}

		template <typename UnderlyingType>
		static auto Process(const std::vector<std::complex<UnderlyingType>>& src_dst) {
			auto dst = src_dst;
			Process(dst);
			return dst;
		}
	};
}
