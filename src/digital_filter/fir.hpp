#pragma once

#include "../common.hpp"
#include "../helpers/is_complex.hpp"

#include <complex>
#include <execution>
#include <vector>

namespace ugsdr {
	template <typename FirImpl>
	class Fir {
		auto& GetImpl() {
			return static_cast<FirImpl&>(*this);
		}

		const auto& GetImpl() const {
			return static_cast<const FirImpl&>(*this);
		}

	public:
		template <Container T>
		void Filter(T& src_dst) {
			GetImpl().Process(src_dst);
		}

		template <Container T>
		[[nodiscard]]
		auto Filter(const T& src) {
			return GetImpl().Process(src);
		}
		
		template <Container T>
		void UpdateWeights(const T& filter_coefficients) {
			GetImpl().Update(filter_coefficients);
		}

		[[nodiscard]]
		const auto& GetWeigts() const {
			return GetImpl().Weights();
		}

	};

	template <typename T, typename WeightsContainer, typename StateContainer/* = std::vector<T>, typename StateContainer = std::vector<T>*/>
	class SequentialFir : public Fir<SequentialFir<T, WeightsContainer, StateContainer>> {
		StateContainer state;
		WeightsContainer weights;

	protected:
		friend class Fir<SequentialFir<T, WeightsContainer, StateContainer>>;

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

		void Update(const WeightsContainer& filter_coefficients) {
			weights = filter_coefficients;
		}

		[[nodiscard]]
		const auto& Weights() const {
			return weights;
		}

	public:
		SequentialFir(std::size_t filter_order) : state(filter_order), weights(filter_order) {}
		SequentialFir(std::vector<T> inp_weights) : state(inp_weights.size()), weights(std::move(inp_weights)) {}
	};

	template<typename T>
	SequentialFir(std::vector<T> weights)->SequentialFir<T, std::vector<T>, std::vector<T>>;
	template<typename T>
	SequentialFir(std::vector<std::complex<T>> weights)->SequentialFir<std::complex<T>, std::vector<std::complex<T>>, std::vector<std::complex<T>>>;

}
