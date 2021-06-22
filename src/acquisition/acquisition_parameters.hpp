#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "acquisition_result.hpp"

#include <numeric>
#include <ranges>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class AccquisitionParametersBase {
	private:
		SignalParametersBase<UnderlyingType>& signal_parameters;
		double doppler_range = 5e3;
		double doppler_step = 0;
		std::vector<std::int32_t> gps_sv;
		std::vector<std::int32_t> gln_sv;

		void InitSatellites() {
			gps_sv.resize(ugsdr::gps_sv_count);
			std::iota(gps_sv.begin(), gps_sv.end(), 0);
			gln_sv.resize(ugsdr::gln_max_frequency - ugsdr::gln_min_frequency);
			std::iota(gln_sv.begin(), gln_sv.end(), ugsdr::gln_min_frequency);
		}

		void ProcessBpsk(const std::vector<std::complex<UnderlyingType>>& translated_signal, std::vector<AcquisitionResult>& dst) const {
			
		}
		
		void ProcessGps(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) const {
			auto intermediate_frequency = 1575.42e6 - signal_parameters.GetCentralFrequency();
		}

		void ProcessGlonass(const std::vector<std::complex<UnderlyingType>>& signal, std::vector<AcquisitionResult>& dst) const {
			for (auto& litera_number : gln_sv) {
				auto intermediate_frequency = 1602e6 + litera_number * 0.5625e6 - signal_parameters.GetCentralFrequency();

			}
		}
		
	public:
		AccquisitionParametersBase(SignalParametersBase<UnderlyingType>& signal_params, double range, double step) :
																														signal_parameters(signal_params),
																														doppler_range(range),
																														doppler_step(step) {
			InitSatellites();
		}

		auto Process(std::size_t ms_offset) const {
			std::vector<AcquisitionResult> dst;
			auto signal = signal_parameters.GetSeveralMs(ms_offset, 5);

			ProcessGps(signal, dst);
			ProcessGlonass(signal, dst);

			return dst;
		}
	};

	using AccquisitionParameters = AccquisitionParametersBase<std::int8_t>;
}
