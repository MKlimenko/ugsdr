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
		Iq_8_plus_8 = 0
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

		void GetPartialSignal(std::size_t length_samples, std::size_t samples_offset, OutputVectorType& dst) {
			switch (file_type) {
			case FileType::Iq_8_plus_8: {
				if constexpr (!std::is_same_v<std::int8_t, UnderlyingType>) {
					auto ptr_start = signal_file.data() + samples_offset * sizeof(std::complex<std::int8_t>);
					dst.resize(length_samples);
					auto convert_wrapper = GetConvertWrapper();
					convert_wrapper(reinterpret_cast<const int8_t*>(ptr_start), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(dst.size()) * 2);
				}
					
				//static thread_local std::vector<std::complex<std::int8_t>> data;
				//std::vector<std::complex<std::int8_t>>* data_ptr;

				//if constexpr (std::is_same_v<std::int8_t, UnderlyingType>)
				//	data_ptr = &dst;
				//else
				//	data_ptr = &data;

				//data_ptr->resize(length_samples);

				//signal_file.seekg(samples_offset * sizeof(std::complex<std::int8_t>));
				//signal_file.read(reinterpret_cast<char*>(data_ptr->data()), length_samples * sizeof((*data_ptr)[0]));

				//if constexpr (!std::is_same_v<std::int8_t, UnderlyingType>) {
				//	dst.resize(length_samples);
				//	auto convert_wrapper = GetConvertWrapper();
				//	convert_wrapper(reinterpret_cast<int8_t*>(data.data()), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(dst.size()) * 2);
				//}

				break;
			}
			default:
				throw std::runtime_error("Unexpected file type");;
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
