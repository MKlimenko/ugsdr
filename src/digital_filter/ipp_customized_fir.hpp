#pragma once

#include "fir.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"
#include "../helpers/is_complex.hpp"
#include "boost/circular_buffer.hpp"

#include <type_traits>

namespace ugsdr {
	template <typename T, typename WeightsContainer, typename StateContainer/* = std::vector<T>, typename StateContainer = std::vector<T>*/>
	class IppCustomizedFir : public Fir<IppCustomizedFir<T, WeightsContainer, StateContainer>> {
		//StateContainer state; boost::circular_buffer
		boost::circular_buffer<T> state;
		WeightsContainer weights;
	
		[[nodiscard]]
		static auto GetDotWrapper() {
			static auto dot_wrapper = plusifier::FunctionWrapper(
				ippsDotProd_32f,
				ippsDotProd_32fc,
				ippsDotProd_64f,
				ippsDotProd_64fc
			);

			return dot_wrapper;
		}

		using IppType = std::conditional_t<is_complex_v<T>,
			typename IppTypeToComplex<underlying_t<T>>::Type,
			T
		>;

	protected:
		friend class Fir<IppCustomizedFir<T, WeightsContainer, StateContainer>>;

		template <typename Ty>
		constexpr static void VerifyType() {
			static_assert(std::is_same_v<Ty, T> || std::is_same_v<std::complex<Ty>, T>, "Please use matching filter weights and input data");
		}

		template <typename Ty>
		void Process(std::vector<Ty>& src_dst) {
			VerifyType<Ty>();
			auto dot_product = GetDotWrapper();
			auto current_state = std::vector<T>(state.size());
			for (std::size_t i = 0; i < src_dst.size(); ++i) {
				state.push_front(src_dst[i]);
				std::copy(std::execution::par_unseq, state.begin(), state.end(), current_state.begin());
				dot_product(reinterpret_cast<const IppType*>(current_state.data()), reinterpret_cast<const IppType*>(weights.data()),
					static_cast<int>(current_state.size()), reinterpret_cast<IppType*>(&src_dst[i]));
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

		void Update(const WeightsContainer& filter_coefficients) {
			weights = filter_coefficients;
		}

		[[nodiscard]]
		const auto& Weights() const {
			return weights;
		}

	public:
		IppCustomizedFir(std::size_t filter_order) : state(filter_order, T{}), weights(filter_order) {}
		IppCustomizedFir(std::vector<T> inp_weights) : state(inp_weights.size(), T{}), weights(std::move(inp_weights)) {}
	};

	template<typename T>
	IppCustomizedFir(std::vector<T> weights)->IppCustomizedFir<T, std::vector<T>, boost::circular_buffer<T>>;
	template<typename T>
	IppCustomizedFir(std::vector<std::complex<T>> weights)->IppCustomizedFir<std::complex<T>, std::vector<std::complex<T>>, boost::circular_buffer<std::complex<T>>>;

}
#endif