#pragma once

#include "mixer.hpp"

namespace ugsdr {
	class BatchMixer : public Mixer<BatchMixer> {
	protected:
		friend class Mixer<BatchMixer>;


		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			if constexpr (std::is_integral_v<UnderlyingType>)
				scale = std::numeric_limits<UnderlyingType>::max();

			double pi_2 = 8 * std::atan(1.0);
			thread_local static std::vector<std::complex<double>> c_exp;
			CheckResize(c_exp, src_dst.size());
			std::iota(c_exp.begin(), c_exp.end(), 0.0);
			std::transform(std::execution::par_unseq, c_exp.begin(), c_exp.end(), c_exp.begin(), [=](auto& val) {
				return val * pi_2 * frequency / sampling_freq + phase;
				});

			std::transform(std::execution::par_unseq, c_exp.begin(), c_exp.end(), src_dst.begin(), src_dst.begin(), [](auto& exp_val, auto& src_val) {
				return src_val * std::complex<UnderlyingType>(exp_val);
				});
		}

	public:
		BatchMixer(double sampling_freq, double frequency, double phase) : Mixer<BatchMixer>(sampling_freq, frequency, phase) {}

	};
}
