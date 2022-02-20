#pragma once

#ifdef HAS_ARRAYFIRE

#pragma pack(push)

#include "is_complex.hpp"

#include <arrayfire.h>
#include <concepts>
#include <optional>
#include <span>
#include <variant>

namespace ugsdr {
	struct ArrayProxy {
	private:
		af::array array;

		auto GetVariant() const {
			std::variant<
				std::vector<float>,
				std::vector<std::complex<float>>,
				std::vector<double>,
				std::vector<std::complex<double>>,
				std::vector<std::int8_t>,
				std::vector<std::int32_t>,
				std::vector<std::uint32_t>,
				std::vector<std::uint8_t>,
				std::vector<std::int64_t>,
				std::vector<std::uint64_t>,
				std::vector<std::int16_t>,
				std::vector<std::uint16_t>
			> dst;

			switch (array.type()) {
			case f32:
				dst = std::vector<float>(static_cast<std::size_t>(array.elements()));
				break;
			case c32:
				dst = std::vector<std::complex<float>>(static_cast<std::size_t>(array.elements()));
				break;
			case f64:
				dst = std::vector<double>(static_cast<std::size_t>(array.elements()));
				break;
			case c64:
				dst = std::vector<std::complex<double>>(static_cast<std::size_t>(array.elements()));
				break;
			case b8:
				dst = std::vector<std::int8_t>(static_cast<std::size_t>(array.elements()));
				break;
			case s32:
				dst = std::vector<std::int32_t>(static_cast<std::size_t>(array.elements()));
				break;
			case u32:
				dst = std::vector<std::uint32_t>(static_cast<std::size_t>(array.elements()));
				break;
			case u8:
				dst = std::vector<std::uint8_t>(static_cast<std::size_t>(array.elements()));
				break;
			case s64:
				dst = std::vector<std::int64_t>(static_cast<std::size_t>(array.elements()));
				break;
			case u64:
				dst = std::vector<std::uint64_t>(static_cast<std::size_t>(array.elements()));
				break;
			case s16:
				dst = std::vector<std::int16_t>(static_cast<std::size_t>(array.elements()));
				break;
			case u16:
				dst = std::vector<std::uint16_t>(static_cast<std::size_t>(array.elements()));
				break;
			case f16:
			default:
				throw std::runtime_error("Unexpected type");
			}
			std::visit([this](auto& vec) {
				array.host(vec.data());
				}, dst);

			return dst;
		}
	public:
		template <typename ...Args>
		ArrayProxy(Args&& ... args) : array(args...) {}

		template <typename T>
		ArrayProxy(const std::vector<T>& vec) : array(vec.size(), vec.data()) {}

		template <typename T>
		ArrayProxy(const std::span<T>& vec) : array(vec.size(), vec.data()) {}

		template <typename T>
		ArrayProxy(const std::complex<T>* data, std::size_t size) {
			if constexpr (std::is_same_v<T, double>)
				array = af::array(size, reinterpret_cast<const af::cdouble*>(data));
			else if constexpr (std::is_same_v<T, float>)
				array = af::array(size, reinterpret_cast<const af::cfloat*>(data));
			else {
				auto converted_vector = std::vector<std::complex<float>>(data, data + size);
				array = af::array(converted_vector.size(), reinterpret_cast<const af::cfloat*>(converted_vector.data()));
			}
		}

		template <typename T>
		ArrayProxy(const std::vector<std::complex<T>>& vec) : ArrayProxy(vec.data(), vec.size()) {}

		operator af::array& () {
			return array;
		}

		operator const af::array& () const {
			return array;
		}
				
		template <typename T = std::complex<double>>
		operator std::vector<T>() const {
			std::vector<T> dst;

			auto type_erased_vector = GetVariant();

			std::visit([&dst](auto&& vec) {
				if constexpr (std::is_same_v<std::remove_reference_t<decltype(vec[0])>, T>) {
					dst = std::move(vec);
					return;
				}

				constexpr bool is_src_complex = is_complex_v<std::remove_reference_t<decltype(vec[0])>>;
				constexpr bool is_dst_complex = is_complex_v<T>;

				if constexpr (is_src_complex && !is_dst_complex) {
					for (auto&& el : vec)
						dst.emplace_back(static_cast<underlying_t<T>>(el.real()));
				}
				else
					for (auto&& el : vec)
						dst.emplace_back(static_cast<std::conditional_t<is_src_complex&& is_dst_complex, T, underlying_t<T>>>(el));
				}, type_erased_vector);

			return dst;
		}


		template <typename T>
		std::optional<std::vector<T>> CopyFromGpu(std::vector<T>& optional_dst) const {
			// type-based map would work nicely here
			switch (array.type()) {
			case c32:
				if (std::is_same_v<T, std::complex<float>>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case c64:
				if (std::is_same_v<T, std::complex<double>>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case f64:
				if (std::is_same_v<T, double>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case f32:
				if (std::is_same_v<T, float>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case s32:
				if (std::is_same_v<T, std::int32_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case u32:
				if (std::is_same_v<T, std::uint32_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case s64:
				if (std::is_same_v<T, std::int64_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case u64:
				if (std::is_same_v<T, std::uint64_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case s16:
				if (std::is_same_v<T, std::int16_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case u16:
				if (std::is_same_v<T, std::uint16_t>) {
					optional_dst.resize(static_cast<std::size_t>(array.elements()));
					array.host(optional_dst.data());
					return std::nullopt;
				}
				break;
			case b8:
			case u8:
			case f16:
			default:
				break;
			}
			return static_cast<std::vector<T>>(*this);
		}

		[[nodiscard]]
		auto size() const {
			return static_cast<std::size_t>(array.elements());
		}
	};

	template <typename T>
	concept AfProxyConcept = std::is_same_v<T, ArrayProxy>;
}

#pragma pack(pop)

#endif
