#pragma once

#include <array>
#include <bitset>
#include <stdexcept>
#include <string>
#include <vector>

namespace ugsdr {
	class MemoryCodes {
	protected:
		static auto CharToInt(char val) {
			if (val >= '0' && val <= '9')
				return val - '0';
			if (val >= 'A' && val <= 'F')
				return val - 'A' + 10;
			if (val >= 'a' && val <= 'f')
				return val - 'a' + 10;

			throw std::runtime_error("Unexpected symbol");
		}

		template <typename T>
		static void Decode(const std::string& memory_code, T* prn, std::size_t memory_code_len) {
			for (std::size_t i = 0; i < memory_code_len; ++i) {
				auto bitset = std::bitset<4>(CharToInt(memory_code[i / 4]));
				auto current_val = 2 * bitset[3 - i % 4] - 1;
				prn[i] = static_cast<T>(current_val);
			}
		}

		template <typename T>
		static void DecodeBoc(const std::string& memory_code, T* prn, std::size_t memory_code_len) {
			for (std::size_t i = 0; i < memory_code_len; ++i) {
				auto bitset = std::bitset<4>(CharToInt(memory_code[i / 4]));
				auto current_val = 2 * bitset[3 - i % 4] - 1;
				prn[2 * i] = static_cast<T>(current_val);
				prn[2 * i + 1] = static_cast<T>(-current_val);
			}
		}
	};
}
