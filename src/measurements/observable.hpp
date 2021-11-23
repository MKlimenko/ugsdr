#pragma once

#include "timescale.hpp"
#include "../common.hpp"
#include "../ephemeris/GpsEphemeris.hpp"
#include "../matched_filter/ipp_matched_filter.hpp"
#include "../math/ipp_abs.hpp"
#include "../resample/resampler.hpp"
#include "../tracking/tracking_parameters.hpp"

#include <limits>
#include <optional>
#include <vector>

namespace ugsdr {
	class Observable final {
		template <typename T, typename E>
		Observable(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& time_scale_ref, std::size_t position, E eph) :
			sv(tracking_result.sv), ephemeris(std::move(eph)), preamble_position(position), time_scale(time_scale_ref) {
			pseudorange.reserve(tracking_result.code_phases.size());
			for (auto& el : tracking_result.code_phases) pseudorange.push_back(el * 1000 / tracking_result.sampling_rate);
			pseudophase = tracking_result.phases;
			doppler = tracking_result.frequencies;
			// snr
		}

		template <typename T>
		static auto GetPreambleGps(std::size_t size) {
			std::vector<std::complex<T>> preamble {
				1, -1, -1, -1, 1, -1, 1, 1
			};
			SequentialUpsampler::Transform(preamble, preamble.size() * 20);
			preamble.resize(size);
			return preamble;
		}

		template <typename T>
		static auto CheckParity(std::span<T> bits_src) {
			auto bits = std::vector<T>(bits_src.begin(), bits_src.end());
			if (bits[1] != 1) {
				for (std::size_t i = 2; i < 26; ++i)
					bits[i] *= -1;
			}
			std::array<T, 6> parity_values;
			// i'd love some fold expressions right here
			parity_values[1 - 1] = bits[1 - 1] * bits[3 - 1] * bits[4 - 1] * bits[5 - 1] * bits[7 - 1] *
				bits[8 - 1] * bits[12 - 1] * bits[13 - 1] * bits[14 - 1] * bits[15 - 1] *
				bits[16 - 1] * bits[19 - 1] * bits[20 - 1] * bits[22 - 1] * bits[25 - 1];

			parity_values[2 - 1] = bits[2 - 1] * bits[4 - 1] * bits[5 - 1] * bits[6 - 1] * bits[8 - 1] *
				bits[9 - 1] * bits[13 - 1] * bits[14 - 1] * bits[15 - 1] * bits[16 - 1] *
				bits[17 - 1] * bits[20 - 1] * bits[21 - 1] * bits[23 - 1] * bits[26 - 1];

			parity_values[3 - 1] = bits[1 - 1] * bits[3 - 1] * bits[5 - 1] * bits[6 - 1] * bits[7 - 1] *
				bits[9 - 1] * bits[10 - 1] * bits[14 - 1] * bits[15 - 1] * bits[16 - 1] *
				bits[17 - 1] * bits[18 - 1] * bits[21 - 1] * bits[22 - 1] * bits[24 - 1];

			parity_values[4 - 1] = bits[2 - 1] * bits[4 - 1] * bits[6 - 1] * bits[7 - 1] * bits[8 - 1] *
				bits[10 - 1] * bits[11 - 1] * bits[15 - 1] * bits[16 - 1] * bits[17 - 1] *
				bits[18 - 1] * bits[19 - 1] * bits[22 - 1] * bits[23 - 1] * bits[25 - 1];

			parity_values[5 - 1] = bits[2 - 1] * bits[3 - 1] * bits[5 - 1] * bits[7 - 1] * bits[8 - 1] *
				bits[9 - 1] * bits[11 - 1] * bits[12 - 1] * bits[16 - 1] * bits[17 - 1] *
				bits[18 - 1] * bits[19 - 1] * bits[20 - 1] * bits[23 - 1] * bits[24 - 1] *
				bits[26 - 1];

			parity_values[6 - 1] = bits[1 - 1] * bits[5 - 1] * bits[7 - 1] * bits[8 - 1] * bits[10 - 1] *
				bits[11 - 1] * bits[12 - 1] * bits[13 - 1] * bits[15 - 1] * bits[17 - 1] *
				bits[21 - 1] * bits[24 - 1] * bits[25 - 1] * bits[26 - 1];

			auto parity_check = std::span(bits.begin() + 26, 6);
			if (std::equal(parity_values.begin(), parity_values.end(), parity_check.begin(), parity_check.end()))
				return true;

			return false;
		}

		template <typename T>
		static auto GetAccumulatedBits(std::span<const T> arr, T val = -1) {
			auto accumulated_bits = IppAccumulator::Transform(arr, 20);
			for (auto& el : accumulated_bits)
				el = el > 0 ? 1 : val;
			return accumulated_bits;
		}

		template <typename T>
		static std::optional<std::size_t> FindPreamblePosition(const std::vector<std::size_t>& indexes, std::span<const T> bits, double first_pseudorange) {
			auto preamble_position = std::numeric_limits<std::size_t>::max();
			for (auto& el : indexes) {
				auto it = std::find_if(indexes.begin(), indexes.end(), [el](auto& ind) { return el - ind == 6000; });
				if (it == indexes.end())
					continue;

				auto accumulated_bits = GetAccumulatedBits(std::span(bits.begin() + *it - 40, 20 * 62));
				if (CheckParity(std::span(accumulated_bits.begin(), 32)) && CheckParity(std::span(accumulated_bits.begin() + 30, 32))) {
					preamble_position = *it;
					if (first_pseudorange < 0.5)
						++preamble_position;
					return preamble_position;
				}
			}

			return std::nullopt;
		}

		template <typename T>
		static auto FindPreambleGps(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) -> std::optional<Observable> {
			std::vector<T> navigation_bits;
			const auto& prompt = tracking_result.prompt;
			navigation_bits.reserve(prompt.size());
			std::transform(prompt.begin(), prompt.end(), std::back_inserter(navigation_bits), [](auto& prompt_value) {
				auto value = prompt_value.real() > 0 ? 1 : -1;
				return static_cast<T>(value);
			});

			const auto preamble = GetPreambleGps<T>(prompt.size());
			const auto bits = std::vector<std::complex<T>>(navigation_bits.begin(), navigation_bits.end());
			auto abs_corr = IppAbs::Transform(IppMatchedFilter::Filter(bits, preamble));

			std::vector<std::size_t> indexes;
			for (std::size_t i = 0; i < abs_corr.size(); ++i)
				if (abs_corr[i] > 153)
					indexes.push_back(i);

			std::vector<T> vals;
			for (auto& el : prompt)
				vals.push_back(el.real());
			auto preamble_position = FindPreamblePosition(indexes, std::span<const T>(vals), tracking_result.code_phases[0] * 1000 / tracking_result.sampling_rate);
			if (!preamble_position)
				return std::nullopt;

			if (preamble_position.value() + 1501 * 20 > navigation_bits.size())
				return std::nullopt;

			auto nav_bits = GetAccumulatedBits(std::span<const T>(navigation_bits.begin() + preamble_position.value() - 20, 1501 * 20), T{});
			auto current_ephemeris = GpsEphemeris(std::span(nav_bits.begin() + 1, nav_bits.end()), nav_bits[0]);

			auto tow_ms = current_ephemeris.tow * 1000;

			receiver_time_scale.UpdateScale(preamble_position.value(), tow_ms, System::Gps);
			
			return Observable(tracking_result, receiver_time_scale, preamble_position.value(), current_ephemeris);
		}

		template <typename T>
		static auto FindPreambleGlonass(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) -> std::optional<Observable> {

			return std::nullopt;
		}
		
		template <typename T>
		static std::optional<Observable> FindPreamble(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) {
			switch (tracking_result.sv.system) {
			case (System::Gps):
				return FindPreambleGps(tracking_result, receiver_time_scale);
			case (System::Glonass):
				//return FindPreambleGlonass(tracking_result, receiver_time_scale);
			default:
				break;
				throw std::runtime_error("Unsupported system");
			}

			return std::nullopt;
		}

		void UpdatePseudorangeGps(std::size_t day_offset) {
			for (std::size_t i = 0; i < pseudorange.size(); ++i) 
				pseudorange[i] += time_scale[i] - (static_cast<std::ptrdiff_t>(i) - preamble_position + std::get<GpsEphemeris>(ephemeris).tow * 1000);
		}

	public:
		Sv sv;
		std::vector<double> pseudorange;
		std::vector<double> pseudophase;
		std::vector<double> doppler;
		std::vector<double> snr;
		std::variant<GpsEphemeris> ephemeris;
		TimeScale& time_scale;

		std::size_t preamble_position = std::numeric_limits<std::size_t>::max();

		template <typename T>
		static auto MakeObservable(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) -> std::optional<Observable> {
			return FindPreamble(tracking_result, receiver_time_scale);
		}

		void UpdatePseudoranges(std::size_t day_offset) {
			switch (sv.system) {
			case (System::Gps):
				UpdatePseudorangeGps(day_offset);
				break;
			default:
				throw std::runtime_error("Unsupported system");
			}
		}
	};
}
