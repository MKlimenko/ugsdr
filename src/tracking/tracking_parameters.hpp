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
		
		double code_phase = 0.0;
		double code_frequency = 0.0;
        double base_code_frequency = 0.0;
		double code_period = 0.0;

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
		TrackingParameters(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend) : TrackingParameters(acquisition, digital_frontend, acquisition.GetAcquiredSignalType()) {}
		TrackingParameters(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, Signal signal) :
			sv(acquisition.sv_number),
			code_phase(acquisition.code_offset),
			carrier_frequency(acquisition.doppler),
			intermediate_frequency(acquisition.intermediate_frequency),
			sampling_rate(digital_frontend.GetSamplingRate(signal)) {

			sv.signal = signal;
			AdaptAcquisitionData(acquisition, digital_frontend);

			translated_signal.resize(static_cast<std::size_t>(sampling_rate / 1e3));

			auto epochs_to_process = digital_frontend.GetNumberOfEpochs(sv.signal);
			
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

		void AdaptAcquisitionData(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend) {
			switch (sv.signal) {
			case Signal::GpsCoarseAcquisition_L1:
				code_frequency = 1.023e6;
				base_code_frequency = 1.023e6;
				code_period = 1;
				break;
			case Signal::GlonassCivilFdma_L1:
				code_frequency = 0.511e6;
				base_code_frequency = 0.511e6;
				code_period = 1;
				break;
			case Signal::Galileo_E1b:
				code_frequency = 1.023e6 * 2; // BOC
				base_code_frequency = 1.023e6 * 2;
				code_period = 4;
				break;
			case Signal::Galileo_E1c:
				code_frequency = 1.023e6 * 2; // BOC
				base_code_frequency = 1.023e6 * 2;
				code_period = 4;
				break;
			case Signal::Galileo_E5aI:
			case Signal::Galileo_E5aQ:
				code_frequency = 10.23e6;
				base_code_frequency = 10.23e6;
				code_period = 1;
				code_phase = std::fmod(code_phase * sampling_rate / digital_frontend.GetSamplingRate(acquisition.GetAcquiredSignalType()),
					code_period * sampling_rate / 1e3);
				carrier_frequency *= 1176.45e6 / 1575.42e6;
				break;
			default:
				break;
			}
		}

		static void AddGps(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
		}

		static void AddGlonass(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
		}

		static void AddGalileo(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
			if (digital_frontend.HasSignal(Signal::Galileo_E1c))
				dst.emplace_back(acquisition, digital_frontend, Signal::Galileo_E1c);
			if (digital_frontend.HasSignal(Signal::Galileo_E5aI))
				dst.emplace_back(acquisition, digital_frontend, Signal::Galileo_E5aI);
			if (digital_frontend.HasSignal(Signal::Galileo_E5aQ))
				dst.emplace_back(acquisition, digital_frontend, Signal::Galileo_E5aQ);

			//auto& latest_params = dst.back();
			//const auto& epoch = digital_frontend.GetEpoch(0).GetSubband(Signal::Galileo_E5aQ);

			//const auto translated = IppMixer::Translate(epoch, latest_params.sampling_rate, -latest_params.carrier_frequency);
			//ugsdr::Add(translated);
			//auto code = SequentialUpsampler::Transform(PrnGenerator<Signal::Galileo_E5aQ>::Get<T>(acquisition.sv_number.id),
			//	static_cast<std::size_t>(digital_frontend.GetSamplingRate(Signal::Galileo_E5aQ) / 1e3));
			//std::rotate(code.begin(), code.begin() + static_cast<std::size_t>(latest_params.code_phase), code.end());
			////ugsdr::Add(PrnGenerator<Signal::Galileo_E5aQ>::Get<T>(acquisition.sv_number.id));
			////ugsdr::Add(code);

			//auto matched_output = IppMatchedFilter::Filter(translated, code);
			//ugsdr::Add(matched_output);
		}

		static void FillTrackingParameters(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			switch (acquisition.sv_number.system) {
			case(System::Gps):
				AddGps(acquisition, digital_frontend, dst);
				break;
			case(System::Glonass):
				AddGlonass(acquisition, digital_frontend, dst);
				break;
			case(System::Galileo):
				AddGalileo(acquisition, digital_frontend, dst);
				break;
			default:
				throw std::runtime_error("Not implemented yet");
			}
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
			code_nco += (K1_DLL * (new_code_error - code_error) + K2_DLL * new_code_error)* (base_code_frequency / 1.023e6);
			code_error = new_code_error;
			code_frequency = base_code_frequency - code_nco;
			code_phase -= sampling_rate / 1000 / 2 * (code_frequency / base_code_frequency - 1);

			code_phases.push_back(code_phase);
			code_frequencies.push_back(code_frequency);
		}

		template <typename Archive>
		void save(Archive& ar) const {
			ar(
				CEREAL_NVP(sv),
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
