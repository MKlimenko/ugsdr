#pragma once

#include <cmath>
#include <complex>
#include <execution>
#include <limits>
#include <numbers>
#include <vector>

namespace ugsdr {
	template <typename MixerImpl>
	class Mixer {
	protected:
		double sampling_rate = 0.0;
		double mixer_frequency = 0.0;
		double mixer_phase = 0.0;

		template <typename T>
		static void FixPhase(T& phase) {
			//phase = std::fmod(phase, 2 * std::numbers::pi_v<T>);
			//if (phase < 0)
			//	phase += 2 * std::numbers::pi_v<T>;
			phase = std::fmod(phase, 2 * 4 * std::atan(1.0));
			if (phase < 0)
				phase += 2 * 4 * std::atan(1.0);
		}

	public:
		Mixer(double sampling_freq, double frequency, double phase) :
			sampling_rate(sampling_freq),
			mixer_frequency(frequency),
			mixer_phase(phase) {}

		template <typename UnderlyingType>
		static void Translate(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			FixPhase(phase);
			if (std::abs(frequency) == 0.0)
				return;
			if (frequency < 0)
				frequency = sampling_freq + frequency;

			MixerImpl::Process(src_dst, sampling_freq, frequency, phase);
		}

		template <typename UnderlyingType>
		static auto Translate(const std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			auto dst = src_dst;
			if (frequency < 0)
				frequency = sampling_freq + frequency;

			FixPhase(phase);
			MixerImpl::Process(dst, sampling_freq, frequency, phase);
			return dst;
		}

		template <typename UnderlyingType>
		void Translate(std::vector<std::complex<UnderlyingType>>& src_dst) {
			Translate(src_dst, sampling_rate, mixer_frequency, mixer_phase);
			auto phase_mod = std::fmod(mixer_frequency / 1000.0, 1.0);
			if (phase_mod < 0)
				phase_mod += 1;
			mixer_phase += 2 * 4 * std::atan(1.0) * phase_mod;
		}

		auto GetFrequency() const {
			return mixer_frequency;
		}

		void SetPhase(double phase) {
			mixer_phase = phase;
		}
	};

	class SequentialMixer : public Mixer<SequentialMixer> {
	protected:
		friend class Mixer<SequentialMixer>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			if constexpr (std::is_integral_v<UnderlyingType>)
				scale = std::numeric_limits<UnderlyingType>::max();

			double pi_2 = 8 * std::atan(1.0);
			for (std::size_t i = 0; i < src_dst.size(); ++i) {
				const double triarg = i * pi_2 * frequency / sampling_freq + phase;
				auto c_exp = scale * std::exp(std::complex<double>(0, triarg));
				src_dst[i] *= std::complex<UnderlyingType>(c_exp);
			}
		}

	public:
		SequentialMixer(double sampling_freq, double frequency, double phase) : Mixer<SequentialMixer>(sampling_freq, frequency, phase) {}
	};
}
