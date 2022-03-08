#pragma once

#ifdef HAS_IPP

#include "stft.hpp"
#include "ipp_dft.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../../external/type_map/include/type_map.hpp"

#include <type_traits>

namespace ugsdr {
	class IppStft : public ShortTimeFourierTransform<IppStft> {
	private:
		static auto GetWindowWrapper() {
			static auto win_wrapper = plusifier::FunctionWrapper(
				ippsWinHann_32fc_I,
				ippsWinHann_64fc_I
			);

			return win_wrapper;
		}

		template <typename T>
		static void ApplyWindow(std::vector<T>& vec) {
			auto window_wrapper = GetWindowWrapper();
			using IppType = typename IppTypeToComplex<underlying_t<T>>::Type;
			window_wrapper(reinterpret_cast<IppType*>(vec.data()), static_cast<int>(vec.size()));
		}

		template <typename T>
		static auto SplitIntoColumns(const std::vector<T>& src, std::size_t window_length, std::size_t overlap_length) {
			using DstType = std::conditional_t<is_complex_v<T>, T, std::complex<T>>;
			auto stride = window_length - overlap_length;
			auto number_of_columns = ((src.size() - overlap_length) / stride);

			auto dst = std::vector(number_of_columns, std::vector<DstType>(window_length));

			std::for_each(std::execution::par_unseq, dst.begin(), dst.end(), [&src, &stride, &window_length, &dst](auto& current_dst) {
				auto offset = std::distance(dst.data(), &current_dst);
				ippsCopy_8u(reinterpret_cast<const Ipp8u*>(src.data() + offset * stride), reinterpret_cast<Ipp8u*>(current_dst.data()), window_length * sizeof(T));
			});

			std::for_each(std::execution::par_unseq, dst.begin(), dst.end(), [](auto& vec) {
				ApplyWindow(vec);
				IppDft::Transform(vec);
			});

			return dst;
		}

		template <typename T>
		static auto OverlapAndAdd(const std::vector<std::vector<T>>& src, std::size_t window_length, std::size_t overlap_length) {
			auto gap_length = window_length - overlap_length;
			std::vector<T> dst(window_length + (src.size() - 1) * gap_length);

			for (std::size_t i = 0; i < src.size(); ++i) {
				auto current_ifft = IppDft::Transform(src[i], true);
				for (std::size_t j = 0; j < current_ifft.size(); ++j)
					dst[i * gap_length + j] += current_ifft[j];
			}

			return dst;
		}

	protected:
		friend class ShortTimeFourierTransform<IppStft>;
		template <typename T>
		static auto Process(const std::vector<T>& src, std::size_t window_length, std::size_t overlap_length) {
			return SplitIntoColumns(src, window_length, overlap_length);
		}

		template <typename T>
		static auto Process(const std::vector<std::vector<T>>& src, std::size_t window_length, std::size_t overlap_length) {
			return OverlapAndAdd(src, window_length, overlap_length);
		}
	public:
	};
}

#endif
