#pragma once

#include "codegen.hpp"

#include <array>
#include <stdexcept>

namespace ugsdr {
	template <std::size_t N>
	class Weil {
	private:
		template <typename T>
		constexpr static auto GenerateLegendre() {
			std::array<T, N> sequence{};

			for (std::size_t k = 1; k < N; ++k) {
				T current_value = 0;
				for (std::size_t x = 0; x < N; ++x) {
					auto current_square = x * x;
					if (current_square % N == k) {
						current_value = 1;
						break;
					}
				}
				sequence[k] = current_value;
			}

			return sequence;
		}

	protected:

		template <std::size_t weil_code_length, typename T>
		static void GenerateBoc(T* prn, std::size_t sv_number, std::size_t(*SecondLegendrePhase)(std::size_t), std::size_t(*WeilTruncationPoint)(std::size_t)) {
			auto legendre = GenerateLegendre<int>();
			auto shifted_legendre = legendre;
			std::rotate(shifted_legendre.begin(), shifted_legendre.begin() + SecondLegendrePhase(sv_number), shifted_legendre.end());
		
			auto weil = legendre;
			for (std::size_t i = 0; i < shifted_legendre.size(); ++i)
				weil[i] ^= shifted_legendre[i];

			std::rotate(weil.begin(), weil.begin() + WeilTruncationPoint(sv_number), weil.end());
			for (std::size_t i = 0; i < weil_code_length; ++i) {
				//prn[i] = static_cast<T>(1 - 2 * weil[i]);
				prn[2 * i] = static_cast<T>(1 - 2 * weil[i]);
				prn[2 * i + 1] = -static_cast<T>(1 - 2 * weil[i]);
			}
		}
	};
}
