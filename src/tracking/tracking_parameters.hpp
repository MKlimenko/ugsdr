#pragma once

#include "../common.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../correlator/ipp_correlator.hpp"
#include "../dfe/dfe.hpp"
#include "../mixer/ipp_mixer.hpp"

#include <complex>
#include <span>

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

		static auto AddWithPhase(std::complex<T> lhs, std::complex<T> rhs, double relative_phase) {
			if (relative_phase > 0.5)
				std::swap(lhs, rhs);

			const auto weighted_lhs = lhs.real() / relative_phase;
			const auto weighted_rhs = rhs.real() / (1.0 - relative_phase);
			if (std::abs(weighted_lhs + weighted_rhs) > std::abs(weighted_lhs))
				return lhs + rhs;
			return lhs - rhs;
		}

		using CorrelatorType = IppCorrelator;

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

		static auto GetCodePeriod(Signal signal) {
			switch (signal) {
			case Signal::GpsCoarseAcquisition_L1:
			case Signal::Gps_L5I:
			case Signal::Gps_L5Q:
			case Signal::GlonassCivilFdma_L1:
			case Signal::GlonassCivilFdma_L2:
			case Signal::Galileo_E5aI:
			case Signal::Galileo_E5aQ:
			case Signal::Galileo_E5bI:
			case Signal::Galileo_E5bQ:
			case Signal::BeiDou_B1I:
			case Signal::NavIC_L5:
			case Signal::SbasCoarseAcquisition_L1:
			case Signal::Sbas_L5I:
			case Signal::Sbas_L5Q:
			case Signal::QzssCoarseAcquisition_L1:
			case Signal::Qzss_L1S:
				return 1;
			case Signal::Gps_L2CM:
				return 20;
			case Signal::Galileo_E1b:
			case Signal::Galileo_E1c:
				return 4;
			case Signal::BeiDou_B1C:
				return 10;
			default:
				throw std::runtime_error("Unexpected signal");
			}

		}

		void AdaptAcquisitionData(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend) {
			code_period = GetCodePeriod(sv.signal);

			switch (sv.signal) {
			case Signal::GpsCoarseAcquisition_L1:
			case Signal::NavIC_L5:
			case Signal::QzssCoarseAcquisition_L1:
			case Signal::Qzss_L1S:
				code_frequency = 1.023e6;
				base_code_frequency = 1.023e6;
				break;
			case Signal::Gps_L2CM:
				code_frequency = 1.023e6 / 2;
				base_code_frequency = 1.023e6 / 2;
				carrier_frequency *= 1227.6e6 / 1575.42e6;
				break;
			case Signal::GlonassCivilFdma_L1:
				code_frequency = 0.511e6;
				base_code_frequency = 0.511e6;
				break;
			case Signal::GlonassCivilFdma_L2:
				code_frequency = 0.511e6;
				base_code_frequency = 0.511e6;
				code_phase = std::fmod(code_phase * sampling_rate / digital_frontend.GetSamplingRate(acquisition.GetAcquiredSignalType()),
					code_period * sampling_rate / 1e3);
				carrier_frequency *= 7.0 / 9;
				break;
			case Signal::Galileo_E1b:
			case Signal::Galileo_E1c:
				code_frequency = 1.023e6 * 2; // BOC
				base_code_frequency = 1.023e6 * 2;
				break;
			case Signal::Gps_L5I:
			case Signal::Gps_L5Q:
			case Signal::Galileo_E5aI:
			case Signal::Galileo_E5aQ:
				code_frequency = 10.23e6;
				base_code_frequency = 10.23e6;
				code_phase = std::fmod(code_phase * sampling_rate / digital_frontend.GetSamplingRate(acquisition.GetAcquiredSignalType()),
					code_period * sampling_rate / 1e3);
				carrier_frequency *= 1176.45e6 / 1575.42e6;
				break;
			case Signal::Galileo_E5bI:
			case Signal::Galileo_E5bQ:
				code_frequency = 10.23e6;
				base_code_frequency = 10.23e6;
				code_phase = std::fmod(code_phase * sampling_rate / digital_frontend.GetSamplingRate(acquisition.GetAcquiredSignalType()),
					code_period * sampling_rate / 1e3);
				carrier_frequency *= 1207.14e6 / 1575.42e6;
				break;
			case Signal::BeiDou_B1I:
				code_frequency = 2.046e6;
				base_code_frequency = 2.046e6;
				break;
			case Signal::BeiDou_B1C:
				code_frequency = 2.046e6;
				base_code_frequency = 2.046e6;
				break;

			case Signal::SbasCoarseAcquisition_L1:
				code_frequency = 1.023e6;
				base_code_frequency = 1.023e6;
				code_phase = std::fmod(code_phase * sampling_rate / digital_frontend.GetSamplingRate(acquisition.GetAcquiredSignalType()),
					code_period * sampling_rate / 1e3);
				carrier_frequency *= 1575.42e6 / 1176.45e6;
				break;
			case Signal::Sbas_L5I:
			case Signal::Sbas_L5Q:
				code_frequency = 10.23e6;
				base_code_frequency = 10.23e6;
				break;
			default:
				throw std::runtime_error("Unexpected signal");
			}
		}

		template <typename Tc>
		static auto CorrelateTranslated(const std::vector<std::complex<T>>& epoch, const TrackingParameters<T>& parameters, const Tc& code, double frequency_offset = 0.0) {
			const auto translated = IppMixer::Translate(epoch, parameters.sampling_rate, -parameters.carrier_frequency + frequency_offset);

			auto local_code = std::vector(code.begin(), code.begin() + translated.size());
			auto batch_size = static_cast<std::size_t>(parameters.code_period * parameters.sampling_rate / 1e3);
			auto matched_output = IppReshapeAndSum::Transform(IppAbs::Transform(IppMatchedFilter::Filter(translated, local_code)), batch_size);

#if 0
			if (frequency_offset == 0.0)
				ugsdr::Add(L"Sv: " + static_cast<std::wstring>(parameters.sv), matched_output);
#endif

			auto it = std::max_element(matched_output.begin(), matched_output.end(), [](auto& lhs, auto& rhs) {
				return std::abs(lhs) < std::abs(rhs);
			});
					
			auto code_offset = static_cast<std::ptrdiff_t>(std::distance(matched_output.begin(), it));
			if (code_offset > batch_size / 2)
				code_offset = code_offset - batch_size;
			return std::make_pair(*it, code_offset);
		}

		template <Signal signal>
		static void AddSignalImpl(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			if (!digital_frontend.HasSignal(signal))
				return;

			auto parameters = TrackingParameters<T>(acquisition, digital_frontend, signal);
			auto samples_per_ms = static_cast<std::size_t>(digital_frontend.GetSamplingRate(signal) / 1e3);

			auto ms_cnt = 3 * PrnGenerator<signal>::GetNumberOfMilliseconds();

			const auto& epoch = digital_frontend.GetSeveralEpochs(0, ms_cnt).GetSubband(signal);
			auto code = SequentialUpsampler::Transform(PrnGenerator<signal>::template Get<T>(parameters.sv.id), samples_per_ms * GetCodePeriod(signal));
			code.resize(epoch.size());
			std::rotate(code.begin(), code.begin() + parameters.code_phase, code.end());

			auto [correlator_value, code_offset] = CorrelateTranslated(epoch, parameters, code);
			auto [correlator_value_offset, tmp] = CorrelateTranslated(epoch, parameters, code, 4e6);

			// will work for now, but this should be executed after L1 lock
			if (std::abs(correlator_value) / std::abs(correlator_value_offset) >= 1.3) {
#if 0
				std::cout << static_cast<std::string>(parameters.sv) << ". Offset: " << code_offset % samples_per_ms << ". Value: " << correlator_value
					<< ". Ratio: " << correlator_value / correlator_value_offset << std::endl;
#endif

				parameters.code_phase -= code_offset;
				dst.push_back(std::move(parameters));
			}
		}

		template <Signal signal, Signal ... signals>
		static void AddSignal(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			AddSignalImpl<signal>(acquisition, digital_frontend, dst);
			if constexpr (sizeof...(signals) != 0)
				AddSignal<signals...>(acquisition, digital_frontend, dst);
		}

		static void AddGps(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			AddSignal<
				Signal::GpsCoarseAcquisition_L1,
				Signal::Gps_L2CM,
				Signal::Gps_L5I,
				Signal::Gps_L5Q
			>(acquisition, digital_frontend, dst);
		}

		static void AddGlonass(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			AddSignal<
				Signal::GlonassCivilFdma_L1, 
				Signal::GlonassCivilFdma_L2
			>(acquisition, digital_frontend, dst);
		}
		
		static void AddGalileo(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
			AddSignal<
				Signal::Galileo_E1b,
				Signal::Galileo_E1c, 
				Signal::Galileo_E5aI,
				Signal::Galileo_E5aQ,
				Signal::Galileo_E5bI,
				Signal::Galileo_E5bQ
			>(acquisition, digital_frontend, dst);
		}

		static void AddBeiDou(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
			//AddSignal<
			//	Signal::BeiDou_B2I
			//>(acquisition, digital_frontend, dst);
		}

		static void AddNavIC(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend);
			//AddSignal<
			//	Signal::NavIC_S			// wish me luck finding the S-band dataset
			//>(acquisition, digital_frontend, dst);
		}

		static void AddSbas(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			AddSignal<
				Signal::SbasCoarseAcquisition_L1,
				Signal::Sbas_L5I
			>(acquisition, digital_frontend, dst);
			dst.emplace_back(acquisition, digital_frontend);
		}

		static void AddQzss(const AcquisitionResult<T>& acquisition, DigitalFrontend<T>& digital_frontend, std::vector<TrackingParameters<T>>& dst) {
			dst.emplace_back(acquisition, digital_frontend); 
			AddSignal<
				Signal::Qzss_L1S
			>(acquisition, digital_frontend, dst);
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
			case(System::BeiDou):
				AddBeiDou(acquisition, digital_frontend, dst);
				break;
			case(System::NavIC):
				AddNavIC(acquisition, digital_frontend, dst);
				break;
			case(System::Sbas):
				AddSbas(acquisition, digital_frontend, dst);
				break;
			case(System::Qzss):
				AddQzss(acquisition, digital_frontend, dst);
				break;
			default:
				throw std::runtime_error("Not implemented yet");
			}
		}

		auto GetSamplesPerChip() const {
			return sampling_rate / base_code_frequency;
		}

		template <typename T1, typename T2>
		auto CorrelateSplit(const T1& translated_signal, const T2& full_code, double current_code_phase) const {
			auto samples_per_ms = static_cast<std::size_t>(sampling_rate / 1e3);
			auto code_period_samples = samples_per_ms * code_period;

			while (current_code_phase < 0)
				current_code_phase += code_period_samples;
			if (current_code_phase > code_period_samples)
				current_code_phase = std::fmod(current_code_phase, code_period_samples);

			auto first_batch_length = static_cast<std::size_t>(std::ceil(code_period_samples - current_code_phase)) % samples_per_ms;
			auto second_batch_length = samples_per_ms - first_batch_length;

			auto first_batch_phase = static_cast<std::size_t>(std::ceil(code_period_samples + current_code_phase));
			auto second_batch_phase = first_batch_phase + first_batch_length;

			auto first = CorrelatorType::Correlate(std::span(translated_signal.begin(), first_batch_length), std::span(full_code.begin() + first_batch_phase, first_batch_length));
			auto second = CorrelatorType::Correlate(std::span(translated_signal.begin() + first_batch_length, second_batch_length), std::span(full_code.begin() + second_batch_phase, second_batch_length));

			return AddWithPhase(first, second, std::fmod(current_code_phase, samples_per_ms) / samples_per_ms);
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
