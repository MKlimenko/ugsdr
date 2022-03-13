#pragma once

#include <complex>
#include <vector>

#include "../mixer/mixer.hpp"
#include "../mixer/ipp_mixer.hpp"

namespace ugsdr {
	template <typename UnderlyingType>
	class AdditionalSignalGenerator {
	public:
		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_real = false) = 0;
		virtual ~AdditionalSignalGenerator() {}
	};

	template <typename UnderlyingType>
	class AdditionalTone : public AdditionalSignalGenerator<UnderlyingType> {
#ifdef HAS_IPP
		using MixerType = IppMixer;
#else
		using MixerType = SequentialMixer;
#endif
		MixerType mixer;
		UnderlyingType interference_level;

	public:
		template <typename T>
		AdditionalTone(double frequency, double sampling_rate, T level) : mixer(sampling_rate, frequency, 0), interference_level(static_cast<UnderlyingType>(level)) {}

		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_real = false) override {
			std::vector<std::complex<UnderlyingType>> tone(src_dst.size(), static_cast<UnderlyingType>(1.0));
			mixer.Translate(tone);

			std::transform(std::execution::par_unseq, src_dst.begin(), src_dst.end(), tone.begin(), src_dst.begin(), 
				[this, is_real](auto& dst, auto& interference) {
					const auto& interference_sample = is_real ? interference.real() : interference;
					return dst + this->interference_level * interference_sample;
			});
		}
	};

	template <typename UnderlyingType>
	class AdditionalChirp : public AdditionalSignalGenerator<UnderlyingType> {
		UnderlyingType interference_level;
		double chirpiness = 0.0;
		double sweep_period = 0.0;
		double fs = 0.0;
		double start_frequency = 0.0;

	public:
		template <typename T>
		AdditionalChirp(double frequency_start, double frequency_stop, double sampling_rate, double chirp_period, T level) :
			interference_level(static_cast<UnderlyingType>(level)), chirpiness((frequency_stop - frequency_start) / chirp_period),
			sweep_period(chirp_period), fs(sampling_rate), start_frequency(frequency_start) {}

		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst, bool is_real = false) override {
			std::transform(std::execution::par_unseq, src_dst.begin(), src_dst.end(), src_dst.begin(),
				[this, &src_dst,is_real](auto& dst) {
					auto current_sample = std::distance(src_dst.data(), &dst);
					auto samples_per_chirp = static_cast<std::size_t>(std::ceil(sweep_period * fs));
					auto current_time = (current_sample % samples_per_chirp) / fs;
					auto triarg = 2 * std::numbers::pi * 
						((chirpiness * current_time * current_time) / 2 + start_frequency * current_time);
					auto interference = std::exp(std::complex<UnderlyingType>(0, triarg));
					const auto& interference_sample = is_real ? interference.real() : interference;
					return dst + this->interference_level * interference_sample;
				});
		}
	};
}
