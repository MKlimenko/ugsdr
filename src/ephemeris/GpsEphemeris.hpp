#pragma once

#include "../common.hpp"

#include <boost/dynamic_bitset.hpp>

#include <numbers>
#include <span>

namespace ugsdr {
	struct GpsEphemeris final {
	private:
		constexpr static inline std::size_t subframe_length = 300;
		constexpr static inline std::size_t word_length = 30;
		constexpr static inline std::size_t word_payload_length = 24;

		template <typename T>
		static void CorrectPhase(std::span<T> word, T last_bit) {
			if (last_bit == static_cast<T>(1))
				for (std::size_t i = 0; i < word_payload_length; ++i)
					word[i] = static_cast<T>(1) - word[i];
		}

		template <typename T>
		static auto bin2dec(std::span<T> full_span, std::size_t offset, std::size_t length) {
			auto data = std::span(full_span.begin() + offset, length);
			boost::dynamic_bitset bit_value(data.size(), 0);
			for (std::size_t i = 0; i < data.size(); ++i)
				bit_value[i] = data[data.size() - i - 1];

			return bit_value.to_ulong();
		}

		template <typename T>
		static auto bin2dec(std::span<T> full_span, std::size_t offset, std::size_t length, std::size_t offset_second, std::size_t length_second) {
			auto first = bin2dec(full_span, offset, length);
			auto second = bin2dec(full_span, offset_second, length_second);

			return (first << length_second) | second;
		}

		template <typename T>
		static double twocomp2dec(std::span<T> full_span, std::size_t offset, std::size_t length) {
			auto raw = bin2dec(full_span, offset, length);
			if (full_span[offset] == static_cast<T>(1))
				return raw - std::pow(2, length);

			return raw;
		}

		template <typename T>
		static double twocomp2dec(std::span<T> full_span, std::size_t offset, std::size_t length, std::size_t offset_second, std::size_t length_second) {
			auto raw = bin2dec(full_span, offset, length, offset_second, length_second);
			if (full_span[offset] == static_cast<T>(1))
				return raw - std::pow(2, length + length_second);

			return raw;
		}

		template <typename T>
		void FillSubframeData(std::span<T> subframe, std::size_t subframe_id) {

			switch (subframe_id) {
			case 1:
				week_number = bin2dec(subframe, 60, 10) + 1024;
				accuracy = bin2dec(subframe, 72, 4);
				health = bin2dec(subframe, 76, 6);
				group_delay = twocomp2dec(subframe, 196, 8) * std::pow(2, -31.0);
				iodc = bin2dec(subframe, 82, 2, 196, 8);
				toc = bin2dec(subframe, 218, 16) * std::pow(2, 4);
				af2 = twocomp2dec(subframe, 240, 8) * std::pow(2, -55);
				af1 = twocomp2dec(subframe, 248, 16) * std::pow(2, -43);
				af0 = twocomp2dec(subframe, 270, 22) * std::pow(2, -31);
				break;
			case 2:
				iode_sf2 = bin2dec(subframe, 60, 8);
				crs = twocomp2dec(subframe, 68, 16) * std::pow(2, -5);
				delta_n = twocomp2dec(subframe, 90, 16) * std::pow(2, -43) * std::numbers::pi;
				m0 = twocomp2dec(subframe, 106, 8, 120, 24) * std::pow(2, -31) * std::numbers::pi;
				cuc = twocomp2dec(subframe, 150, 16) * std::pow(2, -29);
				e = bin2dec(subframe, 166, 8, 180, 24) * std::pow(2, -33);
				cus = twocomp2dec(subframe, 210, 16) * std::pow(2, -29);
				sqrt_A = bin2dec(subframe, 226, 8, 240, 24) * std::pow(2, -19);
				toe = bin2dec(subframe, 270, 16) * std::pow(2, 4);
				break;
			case 3:
				cic = twocomp2dec(subframe, 60, 16) * std::pow(2, -29);
				omega_0 = twocomp2dec(subframe, 76, 8, 90, 24) * std::pow(2, -31) * std::numbers::pi;
				cis = twocomp2dec(subframe, 120, 16) * std::pow(2, -29);
				i_0 = twocomp2dec(subframe, 136, 8, 150, 24) * std::pow(2, -31) * std::numbers::pi;
				crc = twocomp2dec(subframe, 180, 16) * std::pow(2, -5);
				omega = twocomp2dec(subframe, 196, 8, 210, 24) * std::pow(2, -31) * std::numbers::pi;
				omega_dot = twocomp2dec(subframe, 240, 24) * std::pow(2, -43) * std::numbers::pi;
				iode_sf2 = bin2dec(subframe, 270, 8);
				i_dot = twocomp2dec(subframe, 278, 14) * std::pow(2, -43) * std::numbers::pi;

				break;
			default:
				break;
			}
			tow = bin2dec(subframe, 30, 17) * 6 - 30;
		}

	public:
		// subframe 1
		std::size_t week_number = 0;		// modulo-1024
		std::size_t accuracy = 0;			// URA index
		std::size_t health = 0;
		double group_delay = 0.0;
		std::size_t iodc = 0;
		double toc = 0.0;
		double af2 = 0.0;
		double af1 = 0.0;
		double af0 = 0.0;

		//subframe 2
		std::size_t iode_sf2 = 0;
		double crs = 0.0;
		double delta_n = 0.0;
		double m0 = 0.0;
		double cuc = 0.0;
		double e = 0.0;
		double cus = 0.0;
		double sqrt_A = 0.0;
		double toe = 0.0;

		//subframe 3
		double cic = 0.0;
		double omega_0 = 0.0;
		double cis = 0.0;
		double i_0 = 0.0;
		double crc = 0.0;
		double omega = 0.0;
		double omega_dot = 0.0;
		std::size_t iode_sf3 = 0.0;
		double i_dot = 0.0;

		std::size_t tow = 0.0;

		GpsEphemeris() = default;
		template <typename T>
		GpsEphemeris(std::span<T> nav_bits, T last_bit) {
			for (std::size_t i = 0; i < 5; ++i) {
				auto subframe = std::span(nav_bits.begin() + subframe_length * i, subframe_length);
				for (std::size_t j = 0; j < 10; ++j) {
					auto word = std::span(subframe.begin() + 30 * j, word_length);
					CorrectPhase(word, last_bit);
					last_bit = word.back();
				}

				auto subframe_id = bin2dec(subframe, 49, 3);
				FillSubframeData(subframe, subframe_id);
			}

			auto a = 5;
		}
	};
}
