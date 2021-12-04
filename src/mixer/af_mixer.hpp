#pragma once

#ifdef HAS_ARRAYFIRE

#include "arrayfire.h"
#include "mixer.hpp"
#include "../helpers/af_array_proxy.hpp"

namespace ugsdr {
	class AfMixer: public Mixer<AfMixer> {
	protected:
		friend class Mixer<AfMixer>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase = 0) {
			double scale = 1.0;
			auto signal = ArrayProxy(const_cast<const std::vector<std::complex<UnderlyingType>>&>(src_dst));
			auto triarg = ArrayProxy(af::iota(src_dst.size()) * frequency / sampling_freq * 2 * std::numbers::pi + phase);
			auto sine = ArrayProxy(af::sin(triarg));
			auto cosine = ArrayProxy(af::cos(triarg));
			sine = ArrayProxy(af::complex(cosine, sine).as(static_cast<af::array>(signal).type()));
			signal = ArrayProxy(static_cast<af::array>(signal) * sine);

			auto mixer_output_optional = signal.CopyFromGpu(src_dst);

			if (mixer_output_optional.has_value())
				src_dst = std::move(mixer_output_optional.value());
		}
			
	public:
		AfMixer(double sampling_freq, double frequency, double phase) : Mixer<AfMixer>(sampling_freq, frequency, phase) {}
	};
}

#endif
