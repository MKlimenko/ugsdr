#pragma once

#include "timescale.hpp"
#include "../common.hpp"
#include "../ephemeris/GlonassEphemeris.hpp"
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
		static auto GetComplexToImagWrapper() {
			static auto cplx_to_imag_wrapper = plusifier::FunctionWrapper(
				ippsCplxToReal_32fc, ippsCplxToReal_64fc
			);

			return cplx_to_imag_wrapper;
		}

		static auto GetStdDevWrapper() {
			static auto stddev_wrapper = plusifier::FunctionWrapper(
				[](const Ipp32f* pSrc, int len, Ipp32f* pStdDev) { return ippsStdDev_32f(pSrc, len, pStdDev, IppHintAlgorithm::ippAlgHintNone); },
				ippsStdDev_64f
			);
			
			return stddev_wrapper;
		}

		template <typename T>
		void CalculateSnr(const ugsdr::TrackingParameters<T>& tracking_result) {
			static thread_local std::vector<T> real(tracking_result.prompt.size());
			static thread_local std::vector<T> imaginary(tracking_result.prompt.size());
			using IppType = typename IppTypeToComplex<T>::Type;

			auto cplx_to_imag_wrapper = GetComplexToImagWrapper();
			cplx_to_imag_wrapper(reinterpret_cast<const IppType*>(tracking_result.prompt.data()), real.data(), imaginary.data(), static_cast<int>(imaginary.size()));

			T sigma{};
			auto stddev_wrapper = GetStdDevWrapper();
			stddev_wrapper(imaginary.data(), static_cast<int>(imaginary.size()), &sigma);

			snr.resize(tracking_result.prompt.size());
			for(std::size_t i = 0;i<snr.size(); ++i)
				snr[i] = 27 + 20 * std::log10(std::abs(tracking_result.prompt[i].real()) / sigma);
		}
		
		template <typename T, typename E>
		Observable(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& time_scale_ref, std::size_t position, E eph) :
			sv(tracking_result.sv), ephemeris(std::move(eph)), preamble_position(position), time_scale(time_scale_ref) {
			pseudorange.reserve(tracking_result.code_phases.size());
			auto samples_to_ms_rate = 1000 / tracking_result.sampling_rate;
			// we're estimating position in milliseconds, not code periods, so this should work fine
			auto pseudorange_offset = static_cast<std::int32_t>(tracking_result.code_phases[position] * samples_to_ms_rate);
			for (auto& el : tracking_result.code_phases) pseudorange.push_back(el * samples_to_ms_rate - pseudorange_offset);
			auto intermediate_frequency_windup = tracking_result.intermediate_frequency / 1e3;
			for (std::size_t i = 0; i < tracking_result.phases.size(); ++i) pseudophase.push_back(tracking_result.phases[i] + (i + 1) * intermediate_frequency_windup);
			//pseudophase = tracking_result.phases;
			doppler = tracking_result.frequencies;
			for (auto& el : doppler) el -= tracking_result.intermediate_frequency;
			CalculateSnr(tracking_result);
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
		static auto GetPreambleGlonass(std::size_t size) {
			std::vector<std::complex<T>> preamble{
				-1, -1, -1, -1, -1, 1, 1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1, 1, 1, -1, 1, -1, -1, 1,
				1, -1, 1, -1, 1, -1, 1, -1, -1, 1,
			};
			SequentialUpsampler::Transform(preamble, preamble.size() * 10);

			preamble.resize(size);
			return preamble;
		}

		template <typename T>
		static auto GetSquareWaveGlonass(std::size_t size) {
			std::vector<std::complex<T>> square_wave{
				-1, 1
			};
			SequentialUpsampler::Transform(square_wave, square_wave.size() * 10);

			square_wave.resize(size);
			return square_wave;
		}

		template <typename T>
		static auto GetPreambleGalileo(std::size_t size) {
			std::vector<std::complex<T>> preamble{
				1, -1, 1, -1, -1, 1, 1, 1, 1, 1,
			};
			SequentialUpsampler::Transform(preamble, preamble.size() * 4);
			preamble.resize(size);
			return preamble;
		}

		template <typename T>
		static auto GetAccumulatedBits(std::span<const T> arr, T val = -1) {
			auto accumulated_bits = IppAccumulator::Transform(arr, 20);
			for (auto& el : accumulated_bits)
				el = el > static_cast<T>(0) ? static_cast<T>(1) : val;
			return accumulated_bits;
		}

		template <typename T>
		static auto GetAccumulatedBits(std::span<const std::complex<T>> arr, T val = -1) {
			std::vector<T> accumulated_bits;
			for (auto& el : arr)
				accumulated_bits.push_back(el.real());
			IppDecimator::Transform(accumulated_bits, 20);
			for (auto& el : accumulated_bits)
				el = el > static_cast<T>(0) ? static_cast<T>(1) : val;
			return accumulated_bits;
		}

		template <typename T>
		static std::optional<std::size_t> FindPreamblePositionGps(const std::vector<std::size_t>& indexes, std::span<const T> bits, double first_pseudorange) {
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
			auto preamble_position = FindPreamblePositionGps(indexes, std::span<const T>(vals), tracking_result.code_phases[0] * 1000 / tracking_result.sampling_rate);
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
		static std::optional<std::size_t> FindPreamblePositionGlonass(const std::vector<std::size_t>& indexes, std::span<const T> bits, double first_pseudorange) {
			auto preamble_position = std::numeric_limits<std::size_t>::max();
			for (auto& el : indexes) {
				auto it = std::find_if(indexes.begin(), indexes.end(), [el](auto& ind) { return el - ind == 30000; });
				if (it == indexes.end())
					continue;

				preamble_position = *it + 300;
			
				if (first_pseudorange < 0.5)
					++preamble_position;

				return preamble_position;
			}

			return std::nullopt;
		}

		template <typename T>
		static auto FindPreambleGlonass(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) -> std::optional<Observable> {
			std::vector<T> navigation_bits;
			const auto& prompt = tracking_result.prompt;
			navigation_bits.reserve(prompt.size());
			std::transform(prompt.begin(), prompt.end(), std::back_inserter(navigation_bits), [](auto& prompt_value) {
				auto value = prompt_value.real() > 0 ? 1 : -1;
				return static_cast<T>(value);
			});

			const auto preamble = GetPreambleGlonass<T>(prompt.size());
			const auto bits = std::vector<std::complex<T>>(navigation_bits.begin(), navigation_bits.end());
			auto abs_corr = IppAbs::Transform(IppMatchedFilter::Filter(bits, preamble));

			std::vector<std::size_t> indexes;
			for (std::size_t i = 0; i < abs_corr.size(); ++i)
				if (abs_corr[i] > 370)
					indexes.push_back(i);

			std::vector<T> vals;
			for (auto& el : prompt)
				vals.push_back(el.real());
			auto preamble_position = FindPreamblePositionGlonass(indexes, std::span<const T>(vals), tracking_result.code_phases[0] * 1000 / tracking_result.sampling_rate);
			if (!preamble_position)
				return std::nullopt;

			auto next_string_position = preamble_position.value();
			const auto raw_bits = std::vector<std::complex<T>>(bits.begin() + next_string_position, bits.begin() + next_string_position + 10000);
			const auto bits_without_square = IppMatchedFilter::Filter(raw_bits, GetSquareWaveGlonass<T>(raw_bits.size()));
			auto accumulated_bits = GetAccumulatedBits(std::span(bits_without_square));

			if (accumulated_bits[0] < 0) {
				for (std::size_t i = 0; i < accumulated_bits.size(); ++i)
					accumulated_bits[i] = -accumulated_bits[i];
			}

			for (std::size_t current_string = 0; current_string < 5; ++current_string) {
				for (std::size_t j = 0; j < 85; ++j) {
					if (accumulated_bits[j + current_string * 100] > 0)
						continue;
					for (std::size_t i = j + 1; i < 85; ++i)
						accumulated_bits[i + current_string * 100] = -accumulated_bits[i + current_string * 100];
				}
			}
			for(std::size_t i = 0; i < accumulated_bits.size(); ++i)
				accumulated_bits[i] = (accumulated_bits[i] < 0);
			auto current_ephemeris = GlonassEphemeris(std::span(accumulated_bits));

			auto tow_ms = (current_ephemeris.tk - 3 * 60 * 60 + 18) * 1000;

			receiver_time_scale.UpdateScale(preamble_position.value(), tow_ms, System::Glonass);

			return Observable(tracking_result, receiver_time_scale, preamble_position.value(), current_ephemeris);
		}

		template <typename T>
		static auto FindPreambleGalileo(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) -> std::optional<Observable> {
			std::vector<T> navigation_bits;
			const auto& prompt = tracking_result.prompt;
			navigation_bits.reserve(prompt.size());
			std::transform(prompt.begin(), prompt.end(), std::back_inserter(navigation_bits), [](auto& prompt_value) {
				auto value = prompt_value.real() > 0 ? 1 : -1;
				return static_cast<T>(value);
				});

			const auto preamble = GetPreambleGalileo<T>(prompt.size());
			const auto bits = std::vector<std::complex<T>>(navigation_bits.begin(), navigation_bits.end());
			auto abs_corr = IppAbs::Transform(IppMatchedFilter::Filter(bits, preamble));

			std::vector<std::size_t> indexes;
			for (std::size_t i = 0; i < abs_corr.size(); ++i)
				if (abs_corr[i] > 40)
					indexes.push_back(i);
			return std::nullopt;

			std::vector<T> vals;
			for (auto& el : prompt)
				vals.push_back(el.real());
			auto preamble_position = FindPreamblePositionGlonass(indexes, std::span<const T>(vals), tracking_result.code_phases[0] * 1000 / tracking_result.sampling_rate);
			if (!preamble_position)
				return std::nullopt;

			auto next_string_position = preamble_position.value();
			const auto raw_bits = std::vector<std::complex<T>>(bits.begin() + next_string_position, bits.begin() + next_string_position + 10000);
			const auto bits_without_square = IppMatchedFilter::Filter(raw_bits, GetSquareWaveGlonass<T>(raw_bits.size()));
			auto accumulated_bits = GetAccumulatedBits(std::span(bits_without_square));

			if (accumulated_bits[0] < 0) {
				for (std::size_t i = 0; i < accumulated_bits.size(); ++i)
					accumulated_bits[i] = -accumulated_bits[i];
			}

			for (std::size_t current_string = 0; current_string < 5; ++current_string) {
				for (std::size_t j = 0; j < 85; ++j) {
					if (accumulated_bits[j + current_string * 100] > 0)
						continue;
					for (std::size_t i = j + 1; i < 85; ++i)
						accumulated_bits[i + current_string * 100] = -accumulated_bits[i + current_string * 100];
				}
			}
			for (std::size_t i = 0; i < accumulated_bits.size(); ++i)
				accumulated_bits[i] = (accumulated_bits[i] < 0);
			auto current_ephemeris = GlonassEphemeris(std::span(accumulated_bits));

			auto tow_ms = (current_ephemeris.tk - 3 * 60 * 60 + 18) * 1000;

			receiver_time_scale.UpdateScale(preamble_position.value(), tow_ms, System::Glonass);

			return Observable(tracking_result, receiver_time_scale, preamble_position.value(), current_ephemeris);
		}
		
		template <typename T>
		static std::optional<Observable> FindPreamble(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale) {
			switch (tracking_result.sv.system) {
			case (System::Gps):
				return FindPreambleGps(tracking_result, receiver_time_scale);
			case (System::Glonass):
				return FindPreambleGlonass(tracking_result, receiver_time_scale);
			case (System::Galileo):
				return FindPreambleGalileo(tracking_result, receiver_time_scale);
			default:
				throw std::runtime_error("Unsupported system");
			}
		}

		void UpdatePseudorangeGps(std::size_t day_offset) {
			for (std::size_t i = 0; i < pseudorange.size(); ++i)
				pseudorange[i] += time_scale[i] - (static_cast<std::ptrdiff_t>(i) - preamble_position + std::get<GpsEphemeris>(ephemeris).tow * 1000 + 2);
		}

		void UpdatePseudorangeGlonass(std::size_t day_offset) {
			double tk_gps_ms = (std::get<GlonassEphemeris>(ephemeris).tk - 3.0 * 60 * 60 + 18 + day_offset * 86400) * 1000;
			for (std::size_t i = 0; i < pseudorange.size(); ++i)
				pseudorange[i] += time_scale[i] - (static_cast<std::ptrdiff_t>(i) - static_cast<double>(preamble_position) + tk_gps_ms + 2);
		}

	public:
		Sv sv;
		std::vector<double> pseudorange;
		std::vector<double> pseudophase;
		std::vector<double> doppler;
		std::vector<double> snr;
		std::variant<GpsEphemeris, GlonassEphemeris> ephemeris;
		TimeScale& time_scale;
		std::size_t preamble_position = std::numeric_limits<std::size_t>::max();

		template <typename T>
		static void MakeObservable(const ugsdr::TrackingParameters<T>& tracking_result, TimeScale& receiver_time_scale, std::vector<Observable>& observables) {
			auto previous_satellite = std::find_if(observables.begin(), observables.end(), [sv = tracking_result.sv](Observable& obs) {
				return (sv.system == obs.sv.system) && (sv.id == obs.sv.id) && (sv.signal != obs.sv.signal);
			});

			if (previous_satellite == observables.end()) {
				auto optional_obs = FindPreamble(tracking_result, receiver_time_scale);
				if (optional_obs.has_value())
					observables.push_back(std::move(optional_obs.value()));
			}
			else 
				observables.push_back(Observable(tracking_result, receiver_time_scale, previous_satellite->preamble_position, previous_satellite->ephemeris));
			
		}

		void UpdatePseudoranges(std::size_t day_offset) {
			switch (sv.system) {
			case (System::Gps):
				UpdatePseudorangeGps(day_offset);
				break;
			case(System::Glonass):
				UpdatePseudorangeGlonass(day_offset);
				break;
			default:
				throw std::runtime_error("Unsupported system");
			}
		}
	};
}
