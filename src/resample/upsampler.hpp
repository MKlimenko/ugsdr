#pragma once

#include <cmath>
#include <vector>

namespace ugsdr {
	template <typename UpsamplerImpl>
	class Upsampler {
	protected:
	public:
		template <typename T>
		static void Transform(std::vector<T>& src_dst, std::size_t samples) {
			UpsamplerImpl::Process(src_dst, samples);
		}

		template <typename T>
		static auto Transform(const std::vector<T>& src_dst, std::size_t samples) {
			auto dst = src_dst;
			UpsamplerImpl::Process(dst, samples);
			return dst;
		}
	};

	class SequentialUpsampler : public Upsampler<SequentialUpsampler> {
	protected:
		friend class Upsampler<SequentialUpsampler>;

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t samples) {
			std::vector<T> dst(samples);
			auto code_len = src_dst.size();
			for (std::size_t i = 0; i < samples; ++i)
				dst[i] = src_dst[(i * code_len / samples) % code_len];
			src_dst = std::move(dst);
		}

	public:
	};
}
