#pragma once

#include <algorithm>
#include <complex>
#include <execution>
#include <vector>

namespace ugsdr {
	template <typename MaxIndexImpl>
	class MaxIndex {
	protected:
	public:
		template <typename T>
		struct Result {
			T value{};
			std::size_t index{};
		};

		template <typename T>
		static Result<T> Transform(const std::vector<T>& src) {
			return MaxIndexImpl::Process(src);
		}
	};

	class SequentialMaxIndex : public MaxIndex<SequentialMaxIndex> {
	private:
	protected:
		friend class MaxIndex<SequentialMaxIndex>;

		template <typename T>
		static MaxIndex::Result<T> Process(const std::vector<T>& src_dst) {
			T max_value{};
			int max_index{};

			auto max_it = std::max_element(std::execution::par_unseq, src_dst.begin(), src_dst.end());

			auto result = MaxIndex::Result<T>();
			result.value = *max_it;
			result.index = std::distance(src_dst.begin(), max_it);
			return result;
		}
	};
}
