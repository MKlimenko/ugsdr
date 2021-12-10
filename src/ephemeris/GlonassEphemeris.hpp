#pragma once

#include "../common.hpp"
#include "Ephemeris.hpp"

#include <array>
#include <numbers>
#include <span>

namespace ugsdr {
	struct GlonassEphemeris final : public Ephemeris {
	private:
		template <typename T>
		std::size_t CheckHamming(std::span<T> bits) {
			std::vector<std::int32_t> reversed_bits;
			for (auto it = bits.rbegin(); it != bits.rend(); ++it)
				reversed_bits.push_back(static_cast<std::int32_t>(*it));
		
			static auto indexes = std::array{
				std::vector<std::size_t>{ 9, 10, 12, 13, 15, 17, 19, 20, 22, 24, 26, 28, 30, 32, 34, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84 },
				std::vector<std::size_t>{ 9, 11, 12, 14, 15, 18, 19, 21, 22, 25, 26, 29, 30, 33, 34, 36, 37, 40, 41, 44, 45, 48, 49, 52, 53, 56, 57, 60, 61, 64, 65, 67, 68, 71, 72, 75, 76, 79, 80, 83, 84 },
				std::vector<std::size_t>{ 10, 11, 12, 16, 17, 18, 19, 23, 24, 25, 26, 31, 32, 33, 34, 38, 39, 40, 41, 46, 47, 48, 49, 54, 55, 56, 57, 62, 63, 64, 65, 69, 70, 71, 72, 77, 78, 79, 80, 85 },
				std::vector<std::size_t>{ 13, 14, 15, 16, 17, 18, 19, 27, 28, 29, 30, 31, 32, 33, 34, 42, 43, 44, 45, 46, 47, 48, 49, 58, 59, 60, 61, 62, 63, 64, 65, 73, 74, 75, 76, 77, 78, 79, 80 },
				std::vector<std::size_t>{ 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 81, 82, 83, 84, 85 },
				std::vector<std::size_t>{ 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 },
				std::vector<std::size_t>{ 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85 },
				std::vector<std::size_t>{ 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85 },
			};
			std::array<std::int32_t, 8> checksum{};
			std::int32_t first_sum = 0;
			for (std::size_t i = 0; i < 8; ++i) {
				first_sum += reversed_bits[i];
								
				std::int32_t second_sum = 0;
				for (const auto& index : indexes[i]) 
					second_sum += reversed_bits[index - 1];
				
				auto first_el = i == 7 ? (first_sum & 1): reversed_bits[i];

				checksum[i] = first_el ^ (second_sum & 1);
			}

			return std::all_of(checksum.begin(), checksum.end(), [](auto& val) {return val == 0; });
		}

		template <typename T>
		void FillSubframeData(const std::array<std::span<T>, 5>& bits) {
			auto current_bits = bits[0];
			reserved_1 = bin2dec(current_bits, 5, 2);
			P1 = bin2dec(current_bits, 7, 2);
			tk = bin2dec(current_bits, 20, 1) * 30.0 + bin2dec(current_bits, 14, 6) * 60.0 + bin2dec(current_bits, 9, 5) * 60.0 * 60.0;
			x_dot = signmagndec(current_bits, 21, 24) * std::pow(2.0, -20);
			x_dot_dot = signmagndec(current_bits, 45, 5) * std::pow(2.0, -30);
			x = signmagndec(current_bits, 50, 27) * std::pow(2.0, -11);

			current_bits = bits[1];
			bn = bin2dec(current_bits, 5, 3);
			P2 = bin2dec(current_bits, 8, 1);
			tb = bin2dec(current_bits, 9, 7) * 15.0 * 60.0;
			reserved_2 = bin2dec(current_bits, 16, 5);
			y_dot = signmagndec(current_bits, 21, 24) * std::pow(2.0, -20);
			y_dot_dot = signmagndec(current_bits, 45, 5) * std::pow(2.0, -30);
			y = signmagndec(current_bits, 50, 27) * std::pow(2.0, -11);

			current_bits = bits[2];
			P3 = bin2dec(current_bits, 5, 1);
			gamma = signmagndec(current_bits, 6, 11) * std::pow(2.0, -40);
			reserved_3 = bin2dec(current_bits, 17, 1);
			pp = bin2dec(current_bits, 18, 2);
			ln = bin2dec(current_bits, 20, 1);
			z_dot = signmagndec(current_bits, 21, 24) * std::pow(2.0, -20);
			z_dot_dot = signmagndec(current_bits, 45, 5) * std::pow(2.0, -30);
			z = signmagndec(current_bits, 50, 27) * std::pow(2.0, -11);
			if (ln)
				throw std::runtime_error("Frame is marked unhealthy");

			current_bits = bits[3];
			tn = signmagndec(current_bits, 5, 22) * std::pow(2.0, -30);
			delta_tau_n = signmagndec(current_bits, 27, 5) * std::pow(2.0, -30);
			En = bin2dec(current_bits, 32, 5);
			reserved_4 = bin2dec(current_bits, 37, 14);
			P4 = bin2dec(current_bits, 51, 1);
			Ft = bin2dec(current_bits, 52, 4);
			reserved_5 = bin2dec(current_bits, 56, 3);
			nt = bin2dec(current_bits, 59, 11);
			n = bin2dec(current_bits, 70, 5);
			M = bin2dec(current_bits, 75, 2);

			current_bits = bits[4];
			Na = bin2dec(current_bits, 5, 11);
			tau_c = bin2dec(current_bits, 16, 32);
			reserved_6 = bin2dec(current_bits, 48, 1);
			N4 = bin2dec(current_bits, 49, 5);
			tau_gps = bin2dec(current_bits, 54, 22);
			ln = bin2dec(current_bits, 76, 1);

			if (ln)
				throw std::runtime_error("Frame is marked unhealthy");
		}

	public:
		// string 1
		std::size_t reserved_1 = 0;
		std::size_t P1 = 0;
		double tk = 0.0;
		double x_dot = 0.0;
		double x_dot_dot = 0.0;
		double x = 0.0;

		// string 2
		std::size_t bn = 0;
		std::size_t P2 = 0;
		double tb = 0.0;
		std::size_t reserved_2 = 0;
		double y_dot = 0.0;
		double y_dot_dot = 0.0;
		double y = 0.0;

		// string 3
		std::size_t P3 = 0;
		double gamma = 0.0;
		std::size_t reserved_3 = 0;
		std::size_t pp = 0;
		std::size_t ln = 0;
		double z_dot = 0.0;
		double z_dot_dot = 0.0;
		double z = 0.0;

		// string 4
		double tn = 0.0;
		double delta_tau_n = 0.0;
		std::size_t En = 0;
		std::size_t reserved_4;
		std::size_t P4 = 0;
		std::size_t Ft = 0;
		std::size_t reserved_5 = 0;
		std::size_t nt = 0;
		std::size_t n = 0;
		std::size_t M = 0;

		// string 5
		std::size_t Na = 0;
		std::size_t tau_c = 0;
		std::size_t reserved_6 = 0;
		std::size_t N4 = 0;
		std::size_t tau_gps = 0;

		GlonassEphemeris() = default;

		template <typename T>
		GlonassEphemeris(std::span<T> nav_bits) {
			std::array<std::size_t, 5> current_string;
			std::array<std::size_t, 5> string_start;
			std::array<std::span<T>, 5> bits;
			std::array<std::size_t, 5> parity{};

			for (std::size_t i = 0; i < current_string.size(); ++i) {
				current_string[i] = bin2dec(nav_bits, 1 + i * 100, 4);
				if (current_string[i] != i + 1)
					throw std::runtime_error("Unexpected GLONASS string number");
				string_start[i] = bin2dec(nav_bits, i * 100, 1);
				if (string_start[i] != 0)
					throw std::runtime_error("Unexpected GLONASS string start");
				bits[i] = std::span(nav_bits.begin() + i * 100, 85);

				parity[i] = CheckHamming(bits[i]);
				if (parity[i] != 1)
					throw std::runtime_error("Hamming mismatch");
			}

			FillSubframeData(bits);

			auto a = 5;
		}
	};
}
