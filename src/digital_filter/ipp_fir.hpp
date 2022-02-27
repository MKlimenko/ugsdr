#pragma once

#include "fir.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	template <typename T, typename WeightsContainer, typename StateContainer/* = std::vector<T>, typename StateContainer = std::vector<T>*/>
	class IppFir : public Fir<IppFir<T, WeightsContainer, StateContainer>> {
		StateContainer state;
		WeightsContainer weights;

	protected:
		friend class Fir<IppFir<T, WeightsContainer, StateContainer>>;

		template <typename Ty>
		constexpr static void VerifyType() {
			static_assert(std::is_same_v<Ty, T> || std::is_same_v<std::complex<Ty>, T>, "Please use matching filter weights and input data");
		}

		template <typename Ty>
		void Process(std::vector<Ty>& src_dst) {
			VerifyType<Ty>();
			for (std::size_t i = 0; i < src_dst.size(); ++i) {
				std::rotate(state.rbegin(), state.rbegin() + 1, state.rend());
				state[0] = src_dst[i];
				src_dst[i] = std::inner_product(state.begin(), state.end(), weights.begin(), Ty{});
			}
		}

		template <typename Ty>
		[[nodiscard]]
		auto Process(const std::vector<Ty>& src) {
			VerifyType<Ty>();
			auto dst = src;
			Process(dst);
			return dst;
		}
	public:
		IppFir(std::vector<T> weights) : state(weights.size()), weights(std::move(weights)) {
			//ippsFIRSRGetSize(static_cast<int>(weights.size()))
		}
	};

	template<typename T>
	IppFir(std::vector<T> weights)->IppFir<T, std::vector<T>, std::vector<T>>;
	template<typename T>
	IppFir(std::vector<std::complex<T>> weights)->IppFir<std::complex<T>, std::vector<std::complex<T>>, std::vector<std::complex<T>>>;

}
#endif