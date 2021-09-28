#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../acquisition/acquisition_result.hpp"
#include "../resample/upsampler.hpp"
#include "tracking_parameters.hpp"

#include "boost/timer/progress_display.hpp"

#include <map>
#include <span>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	class Tracker final {
		struct Codes final {
		private:
			using UpsamplerType = Upsampler<SequentialUpsampler>;

			template <typename T>
			auto RepeatCodeNTimes(std::vector<T> code, std::size_t repeats) {
				auto ms_size = code.size();
				for (std::size_t i = 1; i < repeats; ++i)
					code.insert(code.end(), code.begin(), code.begin() + ms_size);

				return code;
			}

			using MapType = std::map<Sv, std::vector<UnderlyingType>>;
			
		public:
			MapType codes;

			Codes(double sampling_rate) {
				for (std::int32_t i = 0; i < gps_sv_count; ++i) {
					auto sv = Sv{ System::Gps, i };
					codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Gps>::Get<UnderlyingType>(0), 3),
						static_cast<std::size_t>(3 * sampling_rate / 1e3));
				}

				auto sv = Sv{ System::Glonass, 0 };
				codes[sv] = UpsamplerType::Transform(RepeatCodeNTimes(PrnGenerator<System::Glonass>::Get<UnderlyingType>(0), 3),
					static_cast<std::size_t>(3 * sampling_rate / 1e3));
			}

			const auto& GetMap() const {
				return codes;
			}
		};

		SignalParametersBase<UnderlyingType>& signal_parameters;
		const std::vector<AcquisitionResult>& acquisition_results;

		Codes codes;
		std::vector<TrackingParameters> tracking_parameters;

		void InitTrackingParameters() {
			for (const auto& el : acquisition_results)
				tracking_parameters.emplace_back(el);
		}
		
		template <typename T>
		void TrackSingleSatellite(TrackingParameters& parameters, const std::vector<T>& signal) {
			const auto& code_map = codes.GetMap();
			const auto& full_code = code_map.at(parameters.sv);

		}
		
	public:
		Tracker(SignalParametersBase<UnderlyingType>& signal_params, 
			const std::vector<AcquisitionResult>& acquisition_dst) : signal_parameters(signal_params), acquisition_results(acquisition_dst), codes(signal_parameters.GetSamplingRate()) {
			InitTrackingParameters();
		}

		void Track(std::size_t epochs_to_process) {
			auto timer = boost::timer::progress_display(static_cast<unsigned long>(epochs_to_process));
			
			for(std::size_t i = 0; i < epochs_to_process; ++i, ++timer) {
				auto current_signal_ms = signal_parameters.GetOneMs(i);

				for (auto& current_tracking_parameters : tracking_parameters)
					TrackSingleSatellite(current_tracking_parameters, current_signal_ms);
				
			}
		}
	};
}
