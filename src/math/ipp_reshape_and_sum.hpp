#pragma once

#include "reshape_and_sum.hpp"
#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"


namespace ugsdr {
	class IppReshapeAndSum : public ReshapeAndSum<IppReshapeAndSum> {
	private:
		static auto GetAddWrapper() {
			static auto add_wrapper = plusifier::FunctionWrapper(
				ippsAdd_32f_I, ippsAdd_32fc_I, ippsAdd_64f_I, ippsAdd_64fc_I
			);

			return add_wrapper;
		}

		template <typename TypeToCast, typename T>
		static void ProcessImpl(std::vector<T>& src_dst, std::size_t block_size) {
			auto add_wrapper = GetAddWrapper();
			auto iterations = src_dst.size() / block_size - 1;

			for (std::size_t i = 0; i < iterations; ++i) 
				add_wrapper(reinterpret_cast<const TypeToCast*>(src_dst.data()) + block_size * (i + 1), 
					reinterpret_cast<TypeToCast*>(src_dst.data()), static_cast<int>(block_size));
			
			CheckResize(src_dst, block_size);
		}

	protected:
		friend class ReshapeAndSum<IppReshapeAndSum>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, std::size_t block_size) {
			using IppType = IppTypeToComplex<UnderlyingType>::Type;
			ProcessImpl<IppType>(src_dst, block_size);
		}

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t block_size) {
			ProcessImpl<T>(src_dst, block_size);
		}

		template <typename T>
		static auto Process(const std::vector<T>& src_dst, std::size_t block_size) {
			auto dst = src_dst;
			Process(dst, block_size);
			return dst;
		}
	};
}
