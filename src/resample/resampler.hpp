#pragma once

#include "../common.hpp"

#include <cmath>
#include <vector>
#include "decimator.hpp"
#include "upsampler.hpp"

namespace ugsdr {
	template <typename ResamplerImpl>
	class Resampler {
	protected:
	public:
		template <Container T>
		static void Transform(T& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			ResamplerImpl::Process(src_dst, new_sampling_rate, old_sampling_rate);
		}

		template <Container T>
		static auto Transform(const T& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
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
			auto dst = src_dst;
			Process(dst, new_sampling_rate, old_sampling_rate);
			return dst;
		}

		template <typename T>
		static void Process(std::vector<T>& src_dst, std::size_t new_sampling_rate, std::size_t old_sampling_rate) {
			auto lcm = std::lcm(new_sampling_rate, old_sampling_rate);
			auto new_samples = lcm * src_dst.size() / old_sampling_rate;

			SequentialUpsampler::Transform(src_dst, new_samples);
			Accumulator::Transform(src_dst, lcm / new_sampling_rate);
		}

	public:
	};
}
