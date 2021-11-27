#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../dfe/dfe.hpp"
#include "../resample/upsampler.hpp"
#include "../correlator/af_correlator.hpp"
#include "../correlator/ipp_correlator.hpp"
#include "tracking_parameters.hpp"

#include "../matched_filter/ipp_matched_filter.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../mixer/table_mixer.hpp"

#include "../../external/plusifier/Plusifier.hpp"
#include "../helpers/ipp_complex_type_converter.hpp"

#include "boost/timer/progress_display.hpp"

#include <execution>
#include <map>
#include <numbers>
#include <span>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	struct Codes final {
	private:
		using UpsamplerType = Upsampler<SequentialUpsampler>;
		using MapType = std::map<Sv, std::vector<UnderlyingType>>;

		constexpr static inline Sv glonass_sv = Sv{ 0, System::Glonass, Signal::GlonassCivilFdma_L1 };

	public:
		MapType codes;

		template <Signal signal>
		void FillCodesImpl(const DigitalFrontend<UnderlyingType>& digital_frontend) {
			if (!digital_frontend.HasSignal(signal))
				return;

			auto sampling_rate = digital_frontend.GetSamplingRate(signal);
			auto offset = ugsdr::GetSystemBySignal(signal) == System::Sbas ? ugsdr::sbas_sv_offset :0;
			for (std::int32_t i = offset; i < offset + GetCodesCount(GetSystemBySignal(signal)); ++i) {
				auto sv = Sv{ i, GetSystemBySignal(signal), signal };
				codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<signal>::template Get<UnderlyingType>(i), 3),
					static_cast<std::size_t>(3 * PrnGenerator<signal>::GetNumberOfMilliseconds() * sampling_rate / 1e3));
			}
		}

		template <Signal signal, Signal ... signals>
		void FillCodes(const DigitalFrontend<UnderlyingType>& digital_frontend) {
			FillCodesImpl<signal>(digital_frontend);
			if constexpr(sizeof...(signals) != 0)
				FillCodes<signals...>(digital_frontend);
		}

		Codes(const DigitalFrontend<UnderlyingType>& digital_frontend) {
			FillCodes<
				Signal::GpsCoarseAcquisition_L1,
				Signal::Gps_L2CM,
				Signal::Gps_L5I,
				Signal::Gps_L5Q,
				Signal::GpsCoarseAcquisition_L1,
				Signal::GlonassCivilFdma_L1,
				Signal::Galileo_E1b,
				Signal::Galileo_E1c,
				Signal::Galileo_E5aI,
				Signal::Galileo_E5aQ,
				Signal::Galileo_E5bI,
				Signal::Galileo_E5bQ,
				Signal::Galileo_E6b,
				Signal::Galileo_E6c,
				Signal::BeiDou_B1I,
				Signal::BeiDou_B1C,
				Signal::NavIC_L5,
				Signal::SbasCoarseAcquisition_L1,
				Signal::Sbas_L5I,
				Signal::Sbas_L5Q,
				Signal::QzssCoarseAcquisition_L1,
				Signal::Qzss_L1S,
				Signal::Qzss_L2CM,
				Signal::Qzss_L5I,
				Signal::Qzss_L5Q
			>(digital_frontend);
		}

		const auto& GetCode(Sv sv) const {
			if (sv.system == System::Glonass)
				sv = glonass_sv;
			
			return codes.at(sv);
		}
	};

	template <typename UnderlyingType>
	class Tracker final {
		DigitalFrontend<UnderlyingType>& digital_frontend;
		const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_results;

		Codes<UnderlyingType> codes;
		std::vector<TrackingParameters<UnderlyingType>> tracking_parameters;

		using MatchedFilterType = IppMatchedFilter;
		using MixerType = IppMixer;
		
		void InitTrackingParameters() {
			for (const auto& el : acquisition_results)
				TrackingParameters<UnderlyingType>::FillTrackingParameters(el, digital_frontend, tracking_parameters);
		}

		auto GetEpl(TrackingParameters<UnderlyingType>& parameters, double code_phase, double spacing_chips) {
			auto& translated_signal = parameters.translated_signal;
			MixerType::Translate(translated_signal, parameters.sampling_rate, -parameters.carrier_frequency, -parameters.carrier_phase);
			parameters.UpdatePhase();
			
			const auto& full_code = codes.GetCode(parameters.sv);

			auto spacing_offset = parameters.GetSamplesPerChip() * spacing_chips;
						
			auto output_array = std::array{
				std::make_pair(code_phase + spacing_offset, std::complex<UnderlyingType>{}),
				std::make_pair(code_phase, std::complex<UnderlyingType>{}),
				std::make_pair(code_phase - spacing_offset, std::complex<UnderlyingType>{}),
			};

			std::for_each(std::execution::par_unseq, output_array.begin(), output_array.end(), [&translated_signal, &full_code, &parameters, this](auto& pair) {
				pair.second = parameters.CorrelateSplit(translated_signal, full_code, pair.first);
			});

			return std::make_tuple(output_array[0].second, output_array[1].second, output_array[2].second);
		}


		static auto GetCopyWrapper() {
			static auto copy_wrapper = plusifier::FunctionWrapper(
				ippsCopy_32fc, ippsCopy_64fc
			);

			return copy_wrapper;
		}

		void TrackSingleSatellite(TrackingParameters<UnderlyingType>& parameters, const SignalEpoch<UnderlyingType>& signal_epoch) {
			const auto& signal = signal_epoch.GetSubband(parameters.sv.signal);

			auto copy_wrapper = GetCopyWrapper();
						
			using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
			CheckResize(parameters.translated_signal, signal.size());
			copy_wrapper(reinterpret_cast<const IppType*>(signal.data()), reinterpret_cast<IppType*>(parameters.translated_signal.data()), static_cast<int>(signal.size()));

			auto code_phase = parameters.code_phase - parameters.sampling_rate / 1e3 * (parameters.prompt.size() % parameters.GetCodePeriod());
			auto [early, prompt, late] = GetEpl(parameters, code_phase, 0.25);
			parameters.early.push_back(early);
			parameters.prompt.push_back(prompt);
			parameters.late.push_back(late);

			parameters.Pll(prompt);
			parameters.Dll(early, late);
		}
		
	public:
		Tracker(DigitalFrontend<UnderlyingType>& dfe, 
			const std::vector<AcquisitionResult<UnderlyingType>>& acquisition_dst) :	digital_frontend(dfe), acquisition_results(acquisition_dst),
																						codes(digital_frontend) {
			InitTrackingParameters();
		}

		void Track(std::size_t epochs_to_process) {
			auto timer = boost::timer::progress_display(static_cast<unsigned long>(epochs_to_process));

			for (std::size_t i = 0; i < epochs_to_process; ++i, ++timer) {
				auto& current_signal_ms = digital_frontend.GetEpoch(i);

				std::for_each(std::execution::par_unseq, tracking_parameters.begin(), tracking_parameters.end(),
					[&current_signal_ms, this](auto& current_tracking_parameters) {
						TrackSingleSatellite(current_tracking_parameters, current_signal_ms);
					});
			}
		}

		void Plot() const {
			for (auto& el : tracking_parameters) {
				//ugsdr::Add(L"Early tracking result", el.early);
				ugsdr::Add(static_cast<std::wstring>(el.sv) + L". Prompt tracking result", el.prompt);
				//ugsdr::Add(L"Late tracking result", el.late);
			}			
		}

		const auto& GetTrackingParameters() const {
			return tracking_parameters;
		}
	};
}
