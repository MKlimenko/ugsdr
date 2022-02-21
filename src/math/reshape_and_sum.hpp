#pragma once

#include "../common.hpp"

#include <cmath>
#include <complex>
#include <stdexcept>
#include <vector>

namespace ugsdr {
	template <typename SumImpl>
	class ReshapeAndSum {
	protected:
		template <Container T>
		static void CheckBlockSize(const T& vec, std::size_t block_size) {
			if (auto [dim, rem] = std::div(static_cast<std::int64_t>(vec.size()), block_size); rem != 0)
				throw std::runtime_error("Signal size is not divided by block size");
		}
	public:
		template <Container T>
		static void Transform(T& src_dst, std::size_t block_size) {
			CheckBlockSize(src_dst, block_size);
			SumImpl::Process(src_dst, block_size);
		}

		template <Container T>
		static auto Transform(const T& src, std::size_t block_size) {
			CheckBlockSize(src, block_size);
			return SumImpl::Process(src, block_size);
		}
	};

	class SequentialReshapeAndSum : public ReshapeAndSum<SequentialReshapeAndSum> {
	protected:
		friend class ReshapeAndSum<SequentialReshapeAndSum>;

		template <typename T>
		static void Process(std::vector<T>& src, std::size_t block_size) {
			auto blocks = src.size() / block_size;
			for (std::size_t i = 1; i < blocks; ++i) {
				for (std::size_t j = 0; j < block_size; ++j)
					src[j] += src[j + i * block_size];
			}
			src.resize(block_size);
		}

		template <typename T>
		static auto Process(const std::vector<T>& src, std::size_t block_size) {
			auto dst = src;
			Process(dst, block_size);
			return dst;
		}

	public:
	};
}
