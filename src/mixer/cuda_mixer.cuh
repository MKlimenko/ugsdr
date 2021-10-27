#pragma once

#include "mixer.hpp"

namespace ugsdr {
	class CudaMixer : public Mixer<CudaMixer> {
	protected:
		friend class Mixer<CudaMixer>;

		template <typename UnderlyingType>
		static void Process(std::vector<std::complex<UnderlyingType>>& src_dst, double sampling_freq, double frequency, double phase);

	public:
		CudaMixer(double sampling_freq, double frequency, double phase) : Mixer<CudaMixer>(sampling_freq, frequency, phase) {}
	};
}