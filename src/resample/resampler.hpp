#pragma once

#include <cmath>
#include <vector>

namespace ugsdr {
	template <typename ResamplerImpl>
	class Resampler {
	protected:
	public:
		template <typename T>
		static void Transform(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			ResamplerImpl::Process(src_dst, new_sampling_rate, old_sampling_rate);
		}

		template <typename T>
		static auto Transform(const std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto dst = src_dst;
			ResamplerImpl::Process(dst, new_sampling_rate, old_sampling_rate);
			return dst;
		}
	};

	class SequentialResampler : public Resampler<SequentialResampler> {
	protected:
		friend class Resampler<SequentialResampler>;

		template <typename T>
		static auto Process(const std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			//std::vector<UnderlyingType> dst(samples);
			//auto code_len = src_dst.size();
			//for (std::size_t i = 0; i < samples; ++i)
			//	dst[i] = src_dst[(i * code_len / samples) % code_len];
			//src_dst = std::move(dst);
		}

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			//std::vector<UnderlyingType> dst(samples);
			//auto code_len = src_dst.size();
			//for (std::size_t i = 0; i < samples; ++i)
			//	dst[i] = src_dst[(i * code_len / samples) % code_len];
			//src_dst = std::move(dst);
		}

	public:
	};
}
