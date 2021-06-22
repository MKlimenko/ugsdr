#pragma once

#include <complex>
#include <execution>
#include <filesystem>
#include <fstream>

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

		std::ifstream signal_file;

		using OutputVectorType = std::vector<std::complex<UnderlyingType>>;

		void OpenFile() {
			switch (file_type) {
			case FileType::Iq_8_plus_8:
				signal_file = std::ifstream(signal_file_path, std::ios::binary);
				break;
			default:
				throw std::runtime_error("Unexpected file type");
			}
		}

		void GetPartialSignal(std::size_t length_samples, std::size_t samples_offset, OutputVectorType& dst) {
			switch (file_type) {
			case FileType::Iq_8_plus_8: {
				std::vector<std::complex<std::int8_t>> data;
				std::vector<std::complex<std::int8_t>>* data_ptr;

				if constexpr (std::is_same_v<std::int8_t, UnderlyingType>)
					data_ptr = &dst;
				else
					data_ptr = &data;

				data_ptr->clear();
				data_ptr->resize(length_samples);

				signal_file.seekg(samples_offset * sizeof(std::complex<std::int8_t>));
				signal_file.read(reinterpret_cast<char*>(data_ptr->data()), length_samples * sizeof((*data_ptr)[0]));

				if constexpr (!std::is_same_v<std::int8_t, UnderlyingType>) {
					dst.clear();
					dst.resize(length_samples);
					std::copy(std::execution::par_unseq, data.begin(), data.end(), dst.begin());
				}

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
	};

	using SignalParameters = SignalParametersBase<std::int8_t>;
}
