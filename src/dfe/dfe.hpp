#pragma once

#include "../common.hpp"
#include "../signal_parameters.hpp"
#include "../mixer/ipp_mixer.hpp"
#include "../resample/ipp_resampler.hpp"

#include <algorithm>
#include <execution>
#include <map>
#include <vector>

namespace ugsdr {
	template <typename UnderlyingType>
	struct SignalEpoch final {
	private:
		using VectorType = std::vector<std::complex<UnderlyingType>>;
		
		std::map<Signal, std::size_t> signal_map;
		std::map<std::size_t, VectorType> epoch_data;

	public:
		const auto& GetSubband(Signal signal_type) const {
			return epoch_data.at(signal_map.at(signal_type));
		}
		
		auto& GetSubband(Signal signal_type) {
			auto it = signal_map.find(signal_type);
			if (it == signal_map.end()) {
				auto dst = signal_map.insert({ signal_type, signal_map.size() });
				it = dst.first;
			}
			
			return epoch_data[it->second];
		}

		void Initialize(const std::vector<Signal>& signals, std::size_t sz) {
			auto current_size = epoch_data.size();
			for (auto& signal_type : signals) {
				auto it = signal_map.find(signal_type);
				if (it == signal_map.end()) {
					signal_map.insert({ signal_type, current_size });
				}
			}
			epoch_data.insert({ current_size, VectorType(sz, 0) });
		}
	};

	template <typename UnderlyingType>
	struct Channel final {
	public:
		std::vector<Signal> subbands;
		double sampling_rate = 0.0;
		double central_frequency = 0.0;
		
	private:
		SignalParametersBase<UnderlyingType>& signal_parameters;
		IppMixer mixer;
		IppResampler resampler;

		static auto CentralFrequency(Signal signal) {
			switch (signal) {
			case Signal::GpsCoarseAcquisition_L1:
			case Signal::Galileo_E1b:
			case Signal::Galileo_E1c:
			case Signal::BeiDou_B1C:
			case Signal::SbasCoarseAcquisition_L1:
				return 1575.42e6;
			case Signal::GlonassCivilFdma_L1:
				return 1602e6;
			case Signal::GlonassCivilFdma_L2:
				return 1602e6 * 7 / 9;
			case Signal::BeiDou_B1I:
				return 1561.098e6;
			case Signal::Gps_L2CM:
				return 1227.6e6;
			case Signal::Gps_L5I:
			case Signal::Gps_L5Q:
			case Signal::Galileo_E5aI:
			case Signal::Galileo_E5aQ:
			case Signal::NavIC_L5:
				return 1176.45e6;
			case Signal::Galileo_E5bI:
			case Signal::Galileo_E5bQ:
				return 1207.14e6;
			default:
				throw std::runtime_error("Unexpected signal");
			}
		}
		
		static auto CentralFrequency(const std::vector<Signal>& signals) {
			if (signals.empty())
				throw std::runtime_error("Channel can\'t be empty signalwise");
			
			std::vector<double> central_frequencies;
			for (auto& el : signals)
				central_frequencies.push_back(CentralFrequency(el));

			if (std::adjacent_find(central_frequencies.begin(), central_frequencies.end(), std::not_equal_to<>()) != central_frequencies.end())
				throw std::runtime_error("Central frequencies differ for the passed signals.\nThis may be true for undersampling approaches, but I'm not going to support it for now");

			return central_frequencies[0];
		}
		
		static auto MixerFrequency(SignalParametersBase<UnderlyingType>& signal_params, Signal signal) {
			return signal_params.GetCentralFrequency() - CentralFrequency(signal);
		}

	public:
		Channel(SignalParametersBase<UnderlyingType>& signal_params, Signal signal, double new_sampling_rate) :
			subbands(1, signal), sampling_rate(new_sampling_rate), central_frequency(CentralFrequency(signal)),
			signal_parameters(signal_params), mixer(signal_parameters.GetSamplingRate(), signal_parameters.GetCentralFrequency() - central_frequency, 0) {}

		Channel(SignalParametersBase<UnderlyingType>& signal_params, const std::vector<Signal>& signals, double new_sampling_rate) :
			subbands(signals.begin(), signals.end()), sampling_rate(new_sampling_rate), central_frequency(CentralFrequency(signals)),
			signal_parameters(signal_params), mixer(signal_parameters.GetSamplingRate(), signal_parameters.GetCentralFrequency() - central_frequency, 0) {}
		
		auto GetNumberOfEpochs() const {
			return signal_parameters.GetNumberOfEpochs();
		}
				
		void GetSeveralEpochs(std::size_t epoch_offset, std::size_t epoch_cnt, SignalEpoch<UnderlyingType>& epoch_data) {
			auto& current_vector = epoch_data.GetSubband(subbands[0]);
			signal_parameters.GetSeveralMs(epoch_offset, epoch_cnt, current_vector);

			mixer.Translate(current_vector);
			resampler.Transform(current_vector, static_cast<std::size_t>(sampling_rate), static_cast<std::size_t>(signal_parameters.GetSamplingRate()));
		}

		void GetEpoch(std::size_t epoch_offset, SignalEpoch<UnderlyingType>& epoch_data) {
			GetSeveralEpochs(epoch_offset, 1, epoch_data);
		}
	};

	template <typename UnderlyingType>
	class DigitalFrontend final {		
	public:
		DigitalFrontend(std::vector<Channel<UnderlyingType>> input_channels) : channels(std::move(input_channels)) {}

		DigitalFrontend(Channel<UnderlyingType> channel) {
			channels.push_back(channel);
			signal_epoch.Initialize(channels.back().subbands, static_cast<std::size_t>(channels.back().sampling_rate / 1e3));
			
		}

		template <typename ... Args>
		DigitalFrontend(Channel<UnderlyingType> channel, Args ... rem) : DigitalFrontend(rem...) {
			channels.push_back(channel);
			signal_epoch.Initialize(channels.back().subbands, static_cast<std::size_t>(channels.back().sampling_rate / 1e3));
		}

		bool HasSignal(Signal signal) const {
			auto it = GetChannelIt(signal);

			return it != channels.end() ? true : false;
		}
		
		auto& GetSeveralEpochs(std::size_t epoch_offset, std::size_t epoch_cnt) {
			std::for_each(std::execution::par_unseq, channels.begin(), channels.end(), [epoch_offset, epoch_cnt, this](Channel<UnderlyingType>& channel) {
				channel.GetSeveralEpochs(epoch_offset, epoch_cnt, signal_epoch);
			});

			return signal_epoch;
		}

		auto& GetEpoch(std::size_t epoch_offset) {
			return GetSeveralEpochs(epoch_offset, 1);
		}

		auto GetSamplingRate(Signal signal) const {
			auto it = GetChannel(signal);
			return it->sampling_rate;
		}

		auto GetCentralFrequency(Signal signal) const {
			auto it = GetChannel(signal);
			return it->central_frequency;
		}

		auto GetNumberOfEpochs(Signal signal) const {
			auto it = GetChannel(signal);
			return it->GetNumberOfEpochs();
		}

	private:
		std::vector<Channel<UnderlyingType>> channels;
		SignalEpoch<UnderlyingType> signal_epoch;
		std::size_t current_epoch = 0;

		auto GetChannelIt(Signal signal) const {
			return std::find_if(channels.begin(), channels.end(), [signal](auto& el) {
				return std::find_if(el.subbands.begin(), el.subbands.end(), [signal](auto& subband) {
					return subband == signal;
				}) != el.subbands.end();
			});
		}		
		
		auto GetChannel(Signal signal) const {
			auto it = GetChannelIt(signal);

			if (it == channels.end())
				throw std::runtime_error(R"(Digital frontend doesn't contain requested signal)");

			return it;
		}
	};

	template <typename T, typename ... Args>
	auto MakeChannel(SignalParametersBase<T>& signal_parameters, Args&&...args) {
		return Channel<T>(signal_parameters, args...);
	}
}
