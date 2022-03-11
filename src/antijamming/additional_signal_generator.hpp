#pragma once

#include <complex>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class AdditionalSignalGenerator {
	public:
		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst) = 0;
		virtual ~AdditionalSignalGenerator() {}
	};

	template <typename UnderlyingType>
	class AdditionalTone : public AdditionalSignalGenerator<UnderlyingType> {
	public:
		AdditionalTone(double frequency, double sampling_rate, double level) {}

		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst) override {

		}
	};

	template <typename UnderlyingType>
	class AdditionalChirp : public AdditionalSignalGenerator<UnderlyingType> {
	public:
		virtual void AddSignal(std::vector<std::complex<UnderlyingType>>& src_dst) override {

		}
	};
}
