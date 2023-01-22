#pragma once

#include <cstdint>
#include <cstring>

namespace ugsdr {
	class GaloisLfsr final {
	private:
		std::uint32_t value = 0;
		std::uint32_t poly = 0;
		std::uint32_t reset_period = 0;
		std::uint32_t reset_value = 0;
		std::uint32_t shift_counter = 0;

		std::uint32_t triggers;
		std::uint32_t value_mask;
		std::uint32_t sequence_length;

	public:
		[[nodiscard]]
		std::uint32_t Val() const {
			return value;
		}

		void Val(std::uint32_t v) {
			value = v;
		}

		[[nodiscard]]
		std::uint32_t Poly() const {
			return poly;
		}

		void Poly(std::uint32_t poly_val) {
			poly = poly_val;
			triggers = CountTriggers(poly);
			value_mask = (1 << triggers) - 1;
			sequence_length = reset_period ? reset_period : (1 << triggers) - 1;
		}
		
		[[nodiscard]]
		std::uint32_t ResetPeriod() const {
			return reset_period;
		}

		void ResetPeriod(std::uint32_t reset_period_val) {
			reset_period = reset_period_val;
			sequence_length = reset_period ? reset_period : value_mask;
		}

		[[nodiscard]]
		std::uint32_t ResetVal() const {
			return reset_value;
		}

		void ResetVal(std::uint32_t res_val) {
			reset_value = res_val;
		}
		
		[[nodiscard]]
		std::int32_t Get() const {
			return static_cast<std::int32_t>((value) & 1);
		}

		void Shift() {
			++shift_counter;
			value = ((value >> 1U) ^ ((value & 1U) * poly));
			if (shift_counter == reset_period)
				value = reset_value;
		}

		static std::uint32_t CountTriggers(std::uint32_t poly_val) {
			for (std::uint32_t i = 0; i < 32; ++i)
				if (!poly_val)
					return i;
				else
					poly_val >>= 1;
			return 32;
		}

		[[nodiscard]]
		std::uint32_t CountSequenceLen() const {
			return sequence_length;
		}
	};
}
