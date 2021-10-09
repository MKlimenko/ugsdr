#pragma once

#include "mixer.hpp"
#include "nco.hpp"

#include "gcem.hpp"

#include <array>
#include <cmath>
#include <numbers>

namespace ugsdr {
	class TableMixer final : public Mixer<TableMixer> {
	protected:
		friend class Mixer<TableMixer>;

		template <std::size_t phase_bits, typename T>
		constexpr static auto GetSinCosTable() {
			constexpr auto table_size = 5ull * (1 << (phase_bits)) / 4;
			std::array<T, table_size> table{};

			constexpr auto table_step = 5 * std::numbers::pi / table_size / 2;
			double scale = 1.0;
			if constexpr (std::is_integral_v<T>)
				scale = std::numeric_limits<T>::max();
		
			for (std::size_t i = 0; i < table_size; ++i) {
				auto value = scale * gcem::sin(i * table_step);
				table[i] = std::is_integral_v<T> ? static_cast<T>(std::round(value)) : static_cast<T>(value);
			}
			
			return table;
		}

		// todo: move sincos table into independent class to keep the phase_bits
		template <std::size_t phase_bits, typename T, std::size_t table_size, std::size_t nco_bits>
		static auto GetComplexExp(const std::array<T, table_size>& table, NumericallyControlledOscillator<nco_bits>& nco) {
			constexpr auto cos_offset = table_size / 5;

			auto table_index = (nco.holder & nco.mask) >> (nco_bits - phase_bits);
			nco.holder += nco.adder;

			return std::complex<T>{table[table_index + cos_offset], table[table_index]};
		}

		template <std::size_t phase_bits, typename T, std::size_t table_size>
		static auto GetComplexExp(const std::array<T, table_size>& table, std::uint64_t table_index) {
			constexpr auto cos_offset = table_size / 5;

			return std::complex<T>{table[table_index + cos_offset], table[table_index]};
		}

		
		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			constexpr std::size_t phase_bits = 6;
			constexpr std::size_t nco_bits = 32;

			constexpr auto table = GetSinCosTable<phase_bits, UnderlyingType>();
			NumericallyControlledOscillator<nco_bits> nco(sampling_freq, frequency, phase);

			for (std::size_t i = 0; i < src_dst.size(); ++i)
				//src_dst[i] *= GetComplexExp<phase_bits>(table, ((nco.holder + nco.adder * i) & nco.mask) >> (nco_bits - phase_bits));
				src_dst[i] *= GetComplexExp<phase_bits>(table, nco);
		}

	public:
		TableMixer(double sampling_freq, double frequency, double phase) : Mixer<TableMixer>(sampling_freq, frequency, phase) {}
	};
}
