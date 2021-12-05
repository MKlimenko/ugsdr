#pragma once

#include <complex>
#include <cstddef>
#include <cstdint>
#include <iterator>

namespace ugsdr {
	template <std::size_t offset, typename T>
	class NtlabPackedSpan {
		struct NtlabPacked {
			std::uint8_t reserved_0 : offset = 0;
			std::uint8_t val : 2 = 0;
			std::uint8_t : 0;

			auto GetVal() const {
				std::int8_t dst = val;

				if (dst == 0)
					dst = -1;
				else if (dst == 2)
					dst = -3;

				return static_cast<T>(dst);
			}
		};
		static_assert(sizeof(NtlabPacked) == 1);

		const std::byte* data = nullptr;
		std::size_t signal_samples = 0;

	public:
		class Iterator {
		private:
			const NtlabPacked* packed_sample_ptr = nullptr;

		public:
			using iterator_category = std::contiguous_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::complex<T>;

			Iterator() = default;

			Iterator(const NtlabPacked* sample) :
				packed_sample_ptr(sample) {}

			value_type operator*() const {
				if (!packed_sample_ptr)
					throw std::runtime_error("Nullptr in the NtlabPackedSpan::Iterator");
				return packed_sample_ptr->GetVal();
			}

			Iterator& operator++() {
				++packed_sample_ptr;
				return *this;
			}

			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			Iterator operator+ (difference_type diff) const {
				Iterator dst(packed_sample_ptr + diff);

				return dst;
			}

			difference_type operator-(const Iterator& rhs) const {
				auto ptr_diff = (packed_sample_ptr - rhs.packed_sample_ptr);
				return ptr_diff;
			}

			friend bool operator== (const Iterator& a, const Iterator& b) {
				return (a - b == 0);
			};
			friend bool operator!= (const Iterator& a, const Iterator& b) {
				return !(a == b);
			};
		};

		NtlabPackedSpan(const std::byte* data_ptr, std::size_t samples) : data(data_ptr), signal_samples(samples) {}

		Iterator begin() {
			return Iterator(reinterpret_cast<const NtlabPacked*>(data));
		}
		Iterator end() {
			return Iterator(reinterpret_cast<const NtlabPacked*>(data) + signal_samples);
		}
	};

	template <typename T>
	using NtlabPackedSpanFirst = NtlabPackedSpan<6, T>;
	template <typename T>
	using NtlabPackedSpanSecond = NtlabPackedSpan<4, T>;
	template <typename T>
	using NtlabPackedSpanThird = NtlabPackedSpan<2, T>;
	template <typename T>
	using NtlabPackedSpanFourth = NtlabPackedSpan<0, T>;
}
