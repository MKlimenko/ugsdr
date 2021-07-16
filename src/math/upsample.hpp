#pragma once

#include <cmath>
#include <complex>
#include <execution>
#include <vector>

namespace ugsdr {
	template <typename UpsampleImpl>
	class Upsampler {
	protected:
	public:
		template <typename UnderlyingType>
		static void Resample(std::vector<UnderlyingType>& src_dst, std::size_t samples) {
			UpsampleImpl::Process(src_dst, samples);
		}

		template <typename UnderlyingType>
		static auto Resample(const std::vector<UnderlyingType>& src_dst, std::size_t samples) {
			auto dst = src_dst;
			UpsampleImpl::Process(dst, samples);
			return dst;
		}
	};

	class SequentialUpsampler : public Upsampler<SequentialUpsampler> {
	protected:
		friend class Upsampler<SequentialUpsampler>;

		template <typename UnderlyingType>
		static void Process(std::vector<UnderlyingType>& src_dst, std::size_t samples) {
			std::vector<UnderlyingType> dst(samples);
			auto code_len = src_dst.size();
			for (std::size_t i = 0; i < samples; ++i)
				dst[i] = src_dst[(i * code_len / samples) % code_len];
			src_dst = std::move(dst);
		}

	public:
	};
}
