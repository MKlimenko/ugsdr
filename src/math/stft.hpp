#pragma once

#include "../common.hpp"
#include "dft.hpp"

#include <type_traits>

namespace ugsdr {
	template <typename StftImpl>
	class ShortTimeFourierTransform {
	protected:
	public:
		template <Container T>
		static auto Transform(const T& src, std::size_t window_length = 128, std::size_t overlap_length = 96) {
			return StftImpl::Process(src, window_length, overlap_length);
		}
	};


	class SequentialStft : public ShortTimeFourierTransform<SequentialStft> {
		template <typename T>
		constexpr static auto GetWindow(std::size_t window_length) {
			std::vector<T> dst(window_length);
			for (std::size_t i = 0; i < dst.size(); ++i)
				dst[i] = 0.5 * (1 - std::cos(2.0 * std::numbers::pi * i / dst.size()));

			return dst;
		}

		template <typename T>
		static auto SplitIntoColumns(const std::vector<T>& src, std::size_t window_length, std::size_t overlap_length) {
			using DstType = std::conditional_t<is_complex_v<T>, T, std::complex<T>>;
			auto stride = window_length - overlap_length;
			auto number_of_columns = ((src.size() - overlap_length) / stride);

			auto dst  = std::vector(number_of_columns, std::vector<DstType>(window_length));

			for(std::size_t i = 0; i < dst.size(); ++i)
				std::copy(src.begin() + i * stride, src.begin() + i * stride + window_length, dst[i].begin());

			auto window = GetWindow<underlying_t<T>>(window_length);
			std::for_each(std::execution::par_unseq, dst.begin(), dst.end(), [&window](auto&vec) {
				std::transform(vec.begin(), vec.end(), window.begin(), vec.begin(), std::multiplies<T>());
				SequentialDft::Transform(vec);
			});

			return dst;
		}

	protected:
		friend class ShortTimeFourierTransform<SequentialStft>;
		template <typename T>
		static auto Process(const std::vector<T>& src, std::size_t window_length, std::size_t overlap_length) {
			auto dst = SplitIntoColumns(src, window_length, overlap_length);

			return dst;
		}
	public:
	};

}
