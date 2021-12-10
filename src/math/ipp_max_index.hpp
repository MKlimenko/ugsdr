#pragma once

#include "max_index.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include <type_traits>

namespace ugsdr {
	class IppMaxIndex : public MaxIndex<IppMaxIndex> {
	private:
		static auto GetMaxIndexWrapper() {
			static auto max_index_wrapper = plusifier::FunctionWrapper(
				ippsMaxIndx_32f, ippsMaxIndx_64f
			);

			return max_index_wrapper;
		}

	protected:
		friend class MaxIndex<IppMaxIndex>;

		template <typename T>
		static MaxIndex::Result<T> Process(const std::vector<T>& src_dst) {
			T max_value{};
			int max_index{};

			auto max_index_wrapper = GetMaxIndexWrapper();
			max_index_wrapper(src_dst.data(), static_cast<int>(src_dst.size()), &max_value, &max_index);

			auto result = MaxIndex::Result<T>();
			result.value = max_value;
			result.index = max_index;
			return result;
		}
	};
}

#endif
