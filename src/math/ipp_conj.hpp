#pragma once

#include "conj.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"

#include <type_traits>

namespace ugsdr {
	class IppConj : public ComplexConjugate<IppConj> {
	private:
		static auto GetConjWrapper() {
			static auto conj_wrapper = plusifier::FunctionWrapper(
				ippsConj_16sc, ippsConj_32fc, ippsConj_64fc_A53);

			return conj_wrapper;
		}

		template <typename T>
		struct IppComplexTypeConverter {
			// pseudo type map
			using IppType = std::conditional_t <std::is_same_v<std::int16_t, T>, Ipp16sc,
				std::conditional_t<std::is_same_v<float, T>, Ipp32fc,
				std::conditional_t<std::is_same_v<double, T>, Ipp64fc, void>>>;

			static_assert(!std::is_same_v<IppType, void>, "This type is not supported (yet)");
		};
		
	protected:
		friend class ComplexConjugate<IppConj>;

		template <typename T>
		struct Print;
		
		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst) {
			auto conj_wrapper = GetConjWrapper();
			using IppType = IppComplexTypeConverter<UnderlyingType>::IppType;
						
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
