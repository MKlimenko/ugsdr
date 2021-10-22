#pragma once

#include "../common.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../dfe/dfe.hpp"

namespace ugsdr {
	template <typename T>
	struct TrackingParameters final {
	private:
		constexpr static inline double PLL_NOISE_BANDWIDTH = 25.0;
		constexpr static inline double FLL_NOISE_BANDWIDTH = 250.0;
		constexpr static inline double SUMMATION_INTERVAL_PLL = 0.001;
		constexpr static inline double K1_PLL = SUMMATION_INTERVAL_PLL * ((PLL_NOISE_BANDWIDTH / 0.53) * (PLL_NOISE_BANDWIDTH / 0.53)) + 1.414 * (PLL_NOISE_BANDWIDTH / 0.53);
		constexpr static inline double K2_PLL = 1.414 * (PLL_NOISE_BANDWIDTH / 0.53);
		constexpr static inline double K3_PLL = SUMMATION_INTERVAL_PLL * (FLL_NOISE_BANDWIDTH / 0.25);

		constexpr static inline double DLL_NOISE_BANDWIDTH = 0.7;
		constexpr static inline double DLL_DAMPING_RATIO = 1.0;
		constexpr static inline double DLL_GAIN = 0.1;
		constexpr static inline double WN = DLL_NOISE_BANDWIDTH * 8.0 * DLL_DAMPING_RATIO / (4.0 * DLL_DAMPING_RATIO * DLL_DAMPING_RATIO + 1.0);
		constexpr static inline double TAU1CODE = DLL_GAIN / (WN * WN);
		constexpr static inline double TAU2CODE = 2.0 * DLL_DAMPING_RATIO / WN;
		constexpr static inline double SUMMATION_INTERVAL_DLL = 0.001;
		constexpr static inline double K1_DLL = TAU2CODE / TAU1CODE;
		constexpr static inline double K2_DLL = SUMMATION_INTERVAL_DLL / TAU1CODE;

	public:
		Sv sv;
		Signal signal_type;
		
		double code_phase = 0.0;
		double code_frequency = 0.0;
        double base_code_frequency = 0.0;

        double carrier_phase = 0.0;
        double carrier_frequency = 0.0;
        double intermediate_frequency = 0.0;
        double sampling_rate = 0.0;

		std::vector<double> phases;
		std::vector<double> frequencies;
		std::vector<double> code_phases;
		std::vector<double> code_frequencies;
		std::vector<double> phase_residuals;
		std::vector<double> code_residuals;

		std::vector<std::complex<T>> translated_signal;

		std::vector<std::complex<T>> early;
		std::vector<std::complex<T>> prompt;
		std::vector<std::complex<T>> late;

		std::complex<T> previous_prompt{};
		double carrier_phase_error = 0.0;
		double code_nco = 0.0;
		double code_error = 0.0;

		TrackingParameters() = default;
		TrackingParameters(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend) :
			sv(acquisition.sv_number),
			signal_type(acquisition.GetAcquiredSignalType()),
			code_phase(acquisition.code_offset),
			carrier_frequency(acquisition.doppler),
			intermediate_frequency(acquisition.intermediate_frequency),
			sampling_rate(digital_frontend.GetSamplingRate(signal_type)) {
			
			switch (sv.system) {
			case System::Gps:
				code_frequency = 1.023e6;
				base_code_frequency = 1.023e6;
				break;
			case System::Glonass:
				code_frequency = 0.511e6;
				base_code_frequency = 0.511e6;
				break;
			default:
				break;
			}

			translated_signal.resize(static_cast<std::size_t>(sampling_rate / 1e3));

			auto epochs_to_process = digital_frontend.GetNumberOfEpochs(signal_type);
			
			phases.reserve(epochs_to_process);
			frequencies.reserve(epochs_to_process);
			code_phases.reserve(epochs_to_process);
			code_frequencies.reserve(epochs_to_process);
			phase_residuals.reserve(epochs_to_process);
			code_residuals.reserve(epochs_to_process);

			early.reserve(epochs_to_process);
			prompt.reserve(epochs_to_process);
			late.reserve(epochs_to_process);
		}

		auto GetSamplesPerChip() const {
			return sampling_rate / base_code_frequency;
		}

		void Pll(const std::complex<T>& current_prompt) {
			auto new_phase_error = current_prompt.real() ? atan(current_prompt.imag() / current_prompt.real()) / (std::numbers::pi * 2.0) : 0.0;
			phase_residuals.push_back(new_phase_error);
			auto cross = current_prompt.real() * previous_prompt.imag() - previous_prompt.real() * current_prompt.imag();
			auto dot = std::abs(current_prompt.real() * previous_prompt.real() + previous_prompt.imag() * current_prompt.imag());

			if (dot < 0)
				dot = -dot;

			auto carrier_frequency_error = std::atan2(cross, dot) / std::numbers::pi;
			carrier_frequency += K1_PLL * new_phase_error - K2_PLL * carrier_phase_error - K3_PLL * carrier_frequency_error;
			carrier_phase_error = new_phase_error;
			carrier_phase += new_phase_error * 2 * std::numbers::pi;

			phases.push_back(carrier_phase);
			frequencies.push_back(carrier_frequency);
			previous_prompt = current_prompt;
		}

		void Dll(const std::complex<T>& current_early, const std::complex<T>& current_late) {
			auto early = std::abs(current_early);
			auto late = std::abs(current_late);

			auto new_code_error = 0.0;
			if (early + late != 0)
				new_code_error = (early - late) / (early + late);

			code_residuals.push_back(new_code_error);
			code_nco += K1_DLL * (new_code_error - code_error) + K2_DLL * new_code_error;
			code_error = new_code_error;
			code_frequency = base_code_frequency - code_nco;
			code_phase -= sampling_rate / 1000 / 2 * (code_frequency / base_code_frequency - 1);

			auto samples_per_ms = sampling_rate / 1000;

			while (code_phase < 0)
				code_phase += samples_per_ms;

			code_phases.push_back(code_phase);
			code_frequencies.push_back(code_frequency);
		}

		template <typename Archive>
		void save(Archive& ar) const {
			ar(
				CEREAL_NVP(sv),
				CEREAL_NVP(signal_type),
				CEREAL_NVP(code_phase),
				CEREAL_NVP(code_frequency),
				CEREAL_NVP(base_code_frequency),
				CEREAL_NVP(carrier_phase),
				CEREAL_NVP(carrier_frequency),
				CEREAL_NVP(intermediate_frequency),
				CEREAL_NVP(sampling_rate),
				CEREAL_NVP(phases),
				CEREAL_NVP(frequencies),
				CEREAL_NVP(code_phases),
				CEREAL_NVP(code_frequencies),
				CEREAL_NVP(phase_residuals),
				CEREAL_NVP(code_residuals),
				CEREAL_NVP(translated_signal),
				CEREAL_NVP(early),
				CEREAL_NVP(prompt),
				CEREAL_NVP(late),
				CEREAL_NVP(previous_prompt),
				CEREAL_NVP(carrier_phase_error),
				CEREAL_NVP(code_nco),
				CEREAL_NVP(code_error)
			);
		}

		template <typename Archive>
		void load(Archive& ar) {
			ar(
				sv,
				signal_type,
				code_phase,
				code_frequency,
				base_code_frequency,
				carrier_phase,
				carrier_frequency,
				intermediate_frequency,
				sampling_rate,
				phases,
				frequencies,
				code_phases,
				code_frequencies,
				phase_residuals,
				code_residuals,
				translated_signal,
				early,
				prompt,
				late,
				previous_prompt,
				carrier_phase_error,
				code_nco,
				code_error
			);
		}
	};
}
