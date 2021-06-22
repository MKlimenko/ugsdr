#pragma once

#include <cmath>
#include <complex>
#include <execution>
#include <vector>

namespace ugsdr {
	class Mixer {
	private:
		double sampling_rate = 0.0;
		double mixer_frequency = 0.0;
		double mixer_phase = 0.0;
		
	public:
		Mixer(double sampling_freq, double frequency, double phase) :
																		sampling_rate(sampling_freq),
																		mixer_frequency(frequency),
																		mixer_phase(phase) {}

		template <typename UnderlyingType>
		static void TranslateSequential(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			if constexpr (std::is_integral_v<UnderlyingType>)
				scale = std::numeric_limits<UnderlyingType>::max();

			double pi_2 = 8 * std::atan(1.0);
			for(std::size_t i = 0 ;i < src_dst.size(); ++i) {
				const double triarg = i * pi_2 * frequency / sampling_freq + phase;
				auto c_exp = scale * std::exp(std::complex<double>(0, triarg));
				src_dst[i] *= std::complex<UnderlyingType>(c_exp);
			}
		}

		template <typename UnderlyingType>
		static void Translate(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			if constexpr (std::is_integral_v<UnderlyingType>)
				scale = std::numeric_limits<UnderlyingType>::max();

			double pi_2 = 8 * std::atan(1.0);
			std::vector<std::complex<double>> c_exp(src_dst.size());
			std::iota(c_exp.begin(), c_exp.end(), 0.0);
			std::transform(std::execution::par_unseq, c_exp.begin(), c_exp.end(), c_exp.begin(), [=](auto& val) {
				return val * pi_2 * frequency / sampling_freq + phase;
			});

			std::transform(std::execution::par_unseq, c_exp.begin(), c_exp.end(), src_dst.begin(), src_dst.begin(), [](auto& exp_val, auto& src_val) {
				return src_val * std::complex<UnderlyingType>(exp_val);
			});
		}

		template <typename UnderlyingType>
		static auto TranslateSequential(const std::vector<std::complex<UnderlyingType>>& src, double sampling_freq, double frequency, double phase = 0) {
			auto dst = src;
			TranslateSequential(dst, sampling_freq, frequency, phase);
			return dst;
		}

		template <typename UnderlyingType>
		static auto Translate(const std::vector<std::complex<UnderlyingType>>& src, double sampling_freq, double frequency, double phase = 0) {
			auto dst = src;
			Translate(dst, sampling_freq, frequency, phase);
			return dst;
		}
		

		template <typename UnderlyingType>
		void TranslateSequential(std::vector<std::complex<UnderlyingType>>& src_dst) {
			// todo: increment mixer_phase
			TranslateSequential(src_dst, sampling_rate, mixer_frequency, mixer_phase);
		}
	};
}
