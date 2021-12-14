#pragma once

#include <complex>
#include <cstddef>
#include <cstdint>
#include <iterator>

namespace ugsdr {
	template <typename T>
	class PackedSpan {
		struct Packed {
			std::int8_t im_0 : 2 = 0;
			std::int8_t re_0 : 2 = 0;
			std::int8_t im_1 : 2 = 0;
			std::int8_t re_1 : 2 = 0;

			auto GetVal(bool second) const {
				return std::complex<T>{static_cast<T>((second ? re_1 : re_0) | 0x1), static_cast<T>((second ? im_1 : im_0) | 0x1)};
			}
		};

		const std::byte* data = nullptr;
		std::size_t signal_samples = 0;
	public:
		class Iterator {
		private:
			const Packed* base_packed_sample = nullptr;
			std::ptrdiff_t current_sample = 0;

		public:
			using iterator_category = std::contiguous_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::complex<T>;

			Iterator() = default;

			Iterator(const Packed* sample, std::ptrdiff_t parity = 0) :
				base_packed_sample(sample),
				current_sample(parity) {}

			value_type operator*() const {
				if (!base_packed_sample)
					throw std::runtime_error("Nullptr in the PackedSpan::Iterator");
				return (base_packed_sample + current_sample / 2)->GetVal(current_sample & 1);
			}

			Iterator& operator++() {
				++current_sample;
				return *this;
			}

			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			Iterator operator+ (difference_type diff) const {
				Iterator dst(base_packed_sample, current_sample + diff);

				return dst;
			}

			difference_type operator-(const Iterator& rhs) const {
				auto ptr_diff = (base_packed_sample - rhs.base_packed_sample) * 2;
				ptr_diff += current_sample - rhs.current_sample - 1;
				return ptr_diff;
			}

			friend bool operator== (const Iterator& a, const Iterator& b) {
				return (a - b == 0);
			}
			friend bool operator!= (const Iterator& a, const Iterator& b) {
				return !(a == b);
			}
		};

		PackedSpan(const std::byte* data_ptr, std::size_t samples) : data(data_ptr), signal_samples(samples) {}

		Iterator begin() {
			return Iterator(reinterpret_cast<const Packed*>(data));
		}
		Iterator end() {
			return Iterator(reinterpret_cast<const Packed*>(data + signal_samples / 2), (signal_samples + 1) & 1);
		}
	};
}
