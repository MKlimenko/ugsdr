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
		std::map<Signal, std::vector<std::complex<UnderlyingType>>> epoch_data;

	public:
		const auto& GetSubband(Signal signal_type) const {
			return epoch_data.at(signal_type);
		}
		auto& GetSubband(Signal signal_type) {
			return epoch_data[signal_type];
		}
	};

	template <typename UnderlyingType>
	struct Channel final {
	public:
		Signal subband;
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
				return 1575.42e6;
			case Signal::GlonassCivilFdma_L1:
				return 1602e6;
			default:
				throw std::runtime_error("Unexpected signal");
			}
		}

		static auto MixerFrequency(SignalParametersBase<UnderlyingType>& signal_params, Signal signal) {
			return signal_params.GetCentralFrequency() - CentralFrequency(signal);
		}

	public:
		Channel(SignalParametersBase<UnderlyingType>& signal_params, Signal signal, double new_sampling_rate) :
			subband(signal), sampling_rate(new_sampling_rate), central_frequency(CentralFrequency(signal)),
			signal_parameters(signal_params), mixer(signal_parameters.GetSamplingRate(), signal_parameters.GetCentralFrequency() - central_frequency, 0) {}

		auto GetNumberOfEpochs() const {
			return signal_parameters.GetNumberOfEpochs();
		}
				
		void GetSeveralEpochs(std::size_t epoch_offset, std::size_t epoch_cnt, SignalEpoch<UnderlyingType>& epoch_data) {
			auto& current_vector = epoch_data.GetSubband(subband);
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
		}

		template <typename ... Args>
		DigitalFrontend(Channel<UnderlyingType> channel, Args ... rem) : DigitalFrontend(rem...) {
			channels.push_back(channel);
		}

		bool HasSignal(Signal signal) {
			auto it = GetChannelIt(signal);

			return it != channels.end() ? true : false;
		}
		
		void GetSeveralEpochs(std::size_t epoch_offset, std::size_t epoch_cnt, SignalEpoch<UnderlyingType>& epoch_data) {
			std::for_each(std::execution::par_unseq, channels.begin(), channels.end(), [epoch_offset, epoch_cnt, &epoch_data](Channel<UnderlyingType>& channel) {
				channel.GetSeveralEpochs(epoch_offset, epoch_cnt, epoch_data);
			});
		}

		auto GetSeveralEpochs(std::size_t epoch_offset, std::size_t epoch_cnt) {
			auto dst = SignalEpoch<UnderlyingType>();
			GetSeveralEpochs(epoch_offset, epoch_cnt, dst);
			return dst;
		}

		void GetEpoch(std::size_t epoch_offset, SignalEpoch<UnderlyingType>& epoch_data) {
			GetSeveralEpochs(epoch_offset, 1, epoch_data);
		}

		auto GetEpoch(std::size_t epoch_offset) {
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
		std::size_t current_epoch = 0;

		auto GetChannelIt(Signal signal) const {
			return std::find_if(channels.begin(), channels.end(), [signal](auto& el) {
				return el.subband == signal;
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
