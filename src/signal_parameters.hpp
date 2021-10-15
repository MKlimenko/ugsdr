#pragma once

#include <complex>
#include <execution>
#include <filesystem>
#include <fstream>

#include "boost/iostreams/device/mapped_file.hpp"
#include "ipp.h"

#include "../external/plusifier/Plusifier.hpp"

namespace ugsdr {
	enum class FileType {
		Iq_8_plus_8 = 0,
		Nt1065GrabberFirst,
		Nt1065GrabberSecond,
		Nt1065GrabberThird,
		Nt1065GrabberFourth,
	};

	template <typename UnderlyingType>
	class SignalParametersBase {
	private:
		std::filesystem::path signal_file_path;
		FileType file_type = FileType::Iq_8_plus_8;
		double central_frequency = 1590e6;				// L1 central frequency
		double sampling_rate = 0.0;
		std::size_t number_of_epochs = 0;

		boost::iostreams::mapped_file_source signal_file;

		using OutputVectorType = std::vector<std::complex<UnderlyingType>>;
		
		void OpenFile() {
			switch (file_type) {
			case FileType::Iq_8_plus_8: 
				signal_file.open(signal_file_path.string());
				if (!signal_file.is_open())
					throw std::runtime_error("Unable to open file");
				number_of_epochs = static_cast<std::size_t>(signal_file.size() / (sampling_rate / 1e3) / 2);

				break;
			case FileType::Nt1065GrabberFirst:
			case FileType::Nt1065GrabberSecond:
			case FileType::Nt1065GrabberThird:
			case FileType::Nt1065GrabberFourth:
				signal_file.open(signal_file_path.string());
				if (!signal_file.is_open())
					throw std::runtime_error("Unable to open file");
				number_of_epochs = static_cast<std::size_t>(signal_file.size() / (sampling_rate / 1e3));
				break;
			default:
				throw std::runtime_error("Unexpected file type");
			}
		}

		static auto GetConvertWrapper() {
			static auto convert_wrapper = plusifier::FunctionWrapper(
				ippsConvert_8s32f, ippsCopy_8u
			);

			return convert_wrapper;
		}

		template <typename T>
		void CheckResize(T& vec, std::size_t samples) {
			if (vec.size() != samples)
				vec.resize(samples);
		}

		void GetPartialSignalNt1065(int bit_shift, std::size_t length_samples, std::size_t samples_offset, OutputVectorType& dst) {
			static thread_local std::vector<std::int8_t> unpacked_data;
			CheckResize(unpacked_data, length_samples);
			auto ptr = reinterpret_cast<const Ipp8u*>(signal_file.data()) + samples_offset; // 4.9 n
			ippsRShiftC_8u(ptr, bit_shift, reinterpret_cast<Ipp8u*>(unpacked_data.data()), static_cast<int>(unpacked_data.size())); // 1250n
			ippsAndC_8u_I(0x3, reinterpret_cast<Ipp8u*>(unpacked_data.data()), static_cast<int>(unpacked_data.size()));				// 2406n

			ippsReplaceC_8u(reinterpret_cast<Ipp8u*>(unpacked_data.data()), reinterpret_cast<Ipp8u*>(unpacked_data.data()), static_cast<int>(unpacked_data.size()), 0, 0xff); // 4200n
			ippsReplaceC_8u(reinterpret_cast<Ipp8u*>(unpacked_data.data()), reinterpret_cast<Ipp8u*>(unpacked_data.data()), static_cast<int>(unpacked_data.size()), 2, 0xfd); // 6000n

			static thread_local std::vector<std::int16_t> pseudo_complex_data;
			CheckResize(pseudo_complex_data, length_samples);
			ippsConvert_8s16s(unpacked_data.data(), pseudo_complex_data.data(), static_cast<int>(unpacked_data.size()));	//8000n
			ippsAndC_16u_I(0x00FF, reinterpret_cast<std::uint16_t*>(pseudo_complex_data.data()), static_cast<int>(pseudo_complex_data.size()));	//9800n

			CheckResize(dst, length_samples);
		
			auto convert_wrapper = GetConvertWrapper();
			convert_wrapper(reinterpret_cast<const int8_t*>(pseudo_complex_data.data()), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(length_samples) * 2); //21000n
		}

		void GetPartialSignal(std::size_t length_samples, std::size_t samples_offset, OutputVectorType& dst) {
			switch (file_type) {
			case FileType::Iq_8_plus_8: {
				if constexpr (!std::is_same_v<std::int8_t, UnderlyingType>) {
					auto ptr_start = signal_file.data() + samples_offset * sizeof(std::complex<std::int8_t>);
					if (dst.size() != length_samples)
						dst.resize(length_samples);
					dst.resize(length_samples);
					auto convert_wrapper = GetConvertWrapper();
					convert_wrapper(reinterpret_cast<const int8_t*>(ptr_start), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(dst.size()) * 2);
				}
				break;
			}
			case FileType::Nt1065GrabberFirst:
			case FileType::Nt1065GrabberSecond:
			case FileType::Nt1065GrabberThird:
			case FileType::Nt1065GrabberFourth: {
				GetPartialSignalNt1065(2 * (static_cast<int>(FileType::Nt1065GrabberFourth) - static_cast<int>(file_type)), length_samples, samples_offset, dst);
				break;
			}
			default:
				throw std::runtime_error("Unexpected file type");
			}
		}

	public:
		SignalParametersBase(std::filesystem::path signal_file, FileType type, double central_freq, double sampling_freq) :
			signal_file_path(std::move(signal_file)),
			file_type(type),
			central_frequency(central_freq),
			sampling_rate(sampling_freq) {
			OpenFile();
		}

		void GetSeveralMs(std::size_t ms_offset, std::size_t ms_cnt, OutputVectorType& dst) {
			const auto samples_per_ms = static_cast<std::size_t>(sampling_rate / 1000);
			GetPartialSignal(samples_per_ms * ms_cnt, samples_per_ms * ms_offset, dst);
		}

		auto GetSeveralMs(std::size_t ms_offset, std::size_t ms_cnt) {
			auto dst = OutputVectorType(ms_cnt * static_cast<std::size_t>(sampling_rate / 1e3));
			GetSeveralMs(ms_offset, ms_cnt, dst);
			return dst;
		}

		void GetOneMs(std::size_t ms_offset, OutputVectorType& dst) {
			GetSeveralMs(ms_offset, 1, dst);
		}

		auto GetOneMs(std::size_t ms_offset) {
			return GetSeveralMs(ms_offset, 1);
		}

		auto GetCentralFrequency() const {
			return central_frequency;
		}

		auto GetSamplingRate() const {
			return sampling_rate;
		}

		auto GetNumberOfEpochs() const {
			return number_of_epochs;
		}
	};

	using SignalParameters = SignalParametersBase<float>;
}
