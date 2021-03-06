#pragma once

#include "mixer.hpp"
#include "nco.hpp"

#ifdef HAS_GCEM
#include "gcem.hpp"
#define GCEM_NAMESPACE gcem
#define GCEM_CONSTEXPR constexpr
#else
#define GCEM_NAMESPACE std
#define GCEM_CONSTEXPR 
#endif

#include <array>
#include <cmath>
#include <numbers>

#ifdef HAS_IPP

#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#endif

namespace ugsdr {
	class TableMixer final : public Mixer<TableMixer> {
	protected:
		friend class Mixer<TableMixer>;

		template <std::size_t phase_bits, typename T>
		GCEM_CONSTEXPR static auto GetSinCosTable() {
			constexpr auto table_size = 5ull * (1 << (phase_bits)) / 4;
			std::array<T, table_size> table{};

			constexpr auto table_step = 5 * std::numbers::pi / table_size / 2;
			double scale = 1.0;
			if constexpr (std::is_integral_v<T>)
				scale = std::numeric_limits<T>::max();
		
			for (std::size_t i = 0; i < table_size; ++i) {
				auto value = scale * GCEM_NAMESPACE::sin(i * table_step);
				table[i] = std::is_integral_v<T> ? static_cast<T>(GCEM_NAMESPACE::round(value)) : static_cast<T>(value);
			}
			
			return table;
		}

		// todo: move sincos table into independent class to keep the phase_bits
		template <std::size_t phase_bits, typename T, std::size_t table_size, std::size_t nco_bits>
		static auto GetComplexExp(const std::array<T, table_size>& table, NumericallyControlledOscillator<nco_bits>& nco) {
			constexpr auto cos_offset = table_size / 5;

			auto table_index = nco.holder >> (nco_bits - phase_bits);
			nco.holder += nco.adder;

			return std::complex<T>{table[table_index + cos_offset], table[table_index]};
		}

		template <std::size_t phase_bits, typename T, std::size_t table_size>
		static auto GetComplexExp(const std::array<T, table_size>& table, std::uint64_t table_index) {
			constexpr auto cos_offset = table_size / 5;

			return std::complex<T>{table[table_index + cos_offset], table[table_index]};
		}

#ifdef HAS_IPP

		static auto GetMulWrapper() {
			static auto mul_wrapper = plusifier::FunctionWrapper(
				ippsMul_32fc_I, ippsMul_64fc_I
			);

			return mul_wrapper;
		}
#endif
		
		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			constexpr std::size_t phase_bits = 6;
			constexpr std::size_t nco_bits = 32;

			GCEM_CONSTEXPR auto table = GetSinCosTable<phase_bits, UnderlyingType>();
			NumericallyControlledOscillator<nco_bits> nco(sampling_freq, frequency, phase);

			static thread_local std::vector<std::complex<UnderlyingType>> complex_exp_vec;
			CheckResize(complex_exp_vec, src_dst.size());

			for (std::size_t i = 0; i < complex_exp_vec.size(); ++i)
				complex_exp_vec[i] = GetComplexExp<phase_bits>(table, nco);

#ifdef HAS_IPP
			auto mul_wrapper = GetMulWrapper();

			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			mul_wrapper(reinterpret_cast<IppType*>(complex_exp_vec.data()), reinterpret_cast<IppType*>(src_dst.data()), static_cast<int>(src_dst.size()));
#else
			for (std::size_t i = 0; i < src_dst.size(); ++i)
				src_dst[i] *= complex_exp_vec[i];
#endif
		}

	public:
		TableMixer(double sampling_freq, double frequency, double phase) : Mixer<TableMixer>(sampling_freq, frequency, phase) {}
	};
}
