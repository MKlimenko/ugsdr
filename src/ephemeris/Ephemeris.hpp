#pragma once

#include "../common.hpp"

#include <boost/dynamic_bitset.hpp>

#include <numbers>
#include <span>

namespace ugsdr {
	struct Ephemeris {
	protected:
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
		static double signmagndec(std::span<T> full_span, std::size_t offset, std::size_t length) {
			auto raw = bin2dec(full_span, offset, length);
			if (full_span[offset] == static_cast<T>(1)) 
				return -static_cast<double>(bin2dec(full_span, offset + 1, length - 1));
			
			return raw;
		}
	};
}
