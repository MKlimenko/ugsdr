#pragma once

#include <cstdint>
#include <cstring>

namespace ugsdr {
	template <std::size_t Length>
	class LfsrBase final {
	private:
		constexpr static std::uint32_t mask = 0xFFFFFFFF >> (32 - Length);
		static_assert(Length <= 32, "Current implementation of the linear feedback shift register is limited to 32-bit long");

		std::uint32_t value = 0;
		std::uint32_t poly = 0;
		std::uint32_t inv = 0;
		std::uint32_t outpin = 0;
		std::uint32_t reset_period = 0;
		std::uint32_t reset_value = 0;
		std::uint32_t shift_counter = 0;

		std::uint32_t triggers;
		std::uint32_t value_mask;
		std::uint32_t sequence_length;
		
	public:
		[[nodiscard]]
		std::uint32_t Val() const {
			return val;
		}

		void Val(std::uint32_t v) {
			value = v & MASK;
		}

		[[nodiscard]]
		std::uint32_t Poly() const {
			return poly;
		}

		void Poly(std::uint32_t poly_val) {
			poly = poly_val & mask;
			triggers = CountTriggers(poly);
			value_mask = (1 << triggers) - 1;
			sequence_length = reset_period ? reset_period : (1 << triggers) - 1;
		}

		[[nodiscard]]
		std::uint32_t Inv() const {
			return inv;
		}

		void Inv(std::uint32_t inv_val) {
			this->inv = inv_val;
		}

		[[nodiscard]]
		std::uint32_t Outpin() const {
			return outpin;
		}

		void Outpin(std::uint32_t outpin_val) {
			outpin = outpin_val;
		}

		[[nodiscard]]
		std::uint32_t ResetPeriod() const {
			return reset_period;
		}

		void ResetPeriod(std::uint32_t reset_period_val) {
			reset_period = reset_period_val;
			sequence_length = reset_period ? reset_period : val_mask;
		}

		[[nodiscard]] 
		std::uint32_t ResetVal() const {
			return reset_value;
		}

		void ResetVal(std::uint32_t res_val) {
			reset_value = res_val & mask;
		}

		[[nodiscard]]
		std::uint32_t ShiftCounter() const {
			return shift_counter;
		}

		void ShiftCounter(std::uint32_t shift_cnt) {
			shift_counter = shift_cnt;
		}

		[[nodiscard]]
		std::int32_t Get() const {
			return static_cast<std::int32_t>((value >> outpin) & 1);
		}

		void Shift() {
			++shift_counter;
			if (reset_period && shift_counter == reset_period) {
				shift_counter = 0;
				value = reset_value;
				return;
			}
			std::uint32_t v = value & poly;
			std::uint32_t s = 0;
			for (std::uint32_t i = 0; i < Length; ++i) {
				s += v & 1;
				v >>= 1;
			}
			value <<= 1;
			value |= (s & 1) ^ inv;
			value &= mask;
		}

		static std::uint32_t CountTriggers(std::uint32_t poly_val) {
			for (std::uint32_t i = 0; i < Length; ++i)
				if (!poly_val)
					return i;
				else
					poly_val >>= 1;
			return Length;
		}

		std::uint32_t CountSequenceLen() {
			return sequence_len;
		}

		template <typename T>
		void GenerateSequence(T* seq_ptr, size_t len, bool use_code_offset = false) {
			if (use_code_offset) {
				Shift();
				Shift();
			}
			for (size_t i = 0; i < len; i++) {
				seq_ptr[i] = static_cast<T>(Get());
				Shift();
			}
		}

		template <typename T>
		static void GenerateSequence(const LfsrBase<Length>& lfsr1, const LfsrBase<Length>& lfsr2, T* seq, size_t len, bool use_code_offset = false) {
			LfsrBase<Length> seq_lfsr_1 = lfsr1;
			LfsrBase<Length> seq_lfsr_2 = lfsr2;
			if (use_code_offset) {
				seq_lfsr_1.Shift();
				seq_lfsr_1.Shift();
				seq_lfsr_2.Shift();
				seq_lfsr_2.Shift();
			}
			for (size_t i = 0; i < len; i++) {
				seq[i] = static_cast<T>(seq_lfsr_1.Get() ^ seq_lfsr_2.Get());
				seq_lfsr_1.Shift();
				seq_lfsr_2.Shift();
			}
		}

	};

	using Lfsr = LfsrBase<27>; // maximum GNSS LFSR length (GPS L2C)
}
