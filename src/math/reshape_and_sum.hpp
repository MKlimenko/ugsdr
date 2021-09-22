#pragma once

#include <cmath>
#include <complex>
#include <stdexcept>
#include <vector>

namespace ugsdr {
	template <typename SumImpl>
	class ReshapeAndSum {
	protected:
		template <typename T>
		static void CheckBlockSize(const std::vector<T>& vec, std::size_t block_size) {
			if (auto [dim, rem] = std::div(static_cast<std::int64_t>(vec.size()), block_size); rem != 0)
				throw std::runtime_error("Signal size is not divided by block size");
		}
	public:
		template <typename T>
		static void Transform(std::vector<T>& src_dst, std::size_t block_size) {
			CheckBlockSize(src_dst, block_size);
			SumImpl::Process(src_dst, block_size);
		}

		template <typename T>
		static auto Transform(const std::vector<T>& src, std::size_t block_size) {
			CheckBlockSize(src, block_size);
			return SumImpl::Process(src, block_size);
		}
	};
}
