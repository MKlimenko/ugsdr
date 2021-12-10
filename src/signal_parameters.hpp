#pragma once

#include <complex>
#include <execution>
#include <filesystem>
#include <fstream>

#include "boost/iostreams/device/mapped_file.hpp"

#include "common.hpp"
#include "helpers/BbpPackedSpan.hpp"
#include "helpers/NtlabPackedSpan.hpp"

#ifdef HAS_IPP

#include "ipp.h"
#include "helpers/ipp_complex_type_converter.hpp"
#include "../external/plusifier/Plusifier.hpp"

#endif

namespace ugsdr {
	enum class FileType {
		Iq_8_plus_8 = 0,
		Iq_16_plus_16,
		Real_8,
		Nt1065GrabberFirst,
		Nt1065GrabberSecond,
		Nt1065GrabberThird,
		Nt1065GrabberFourth,
		BbpDdc
	};

	template <typename UnderlyingType>
	class SignalParametersBase {
	private:
		std::filesystem::path signal_file_path;
		FileType file_type = FileType::Iq_8_plus_8;
		double central_frequency = 1590e6;				// L1 central frequency
		double sampling_rate = 0.0;
		std::size_t number_of_epochs = 0;
		std::size_t epoch_size_bytes = 0;

		boost::iostreams::mapped_file_source signal_file;

		using OutputVectorType = std::vector<std::complex<UnderlyingType>>;
		
		void VerifyHeaders() const {
			auto last_header = *reinterpret_cast<const std::uint64_t*>(signal_file.data()) & ((1 << 24) - 1);
			for (std::size_t i = 1; i < number_of_epochs; ++i) {
				auto ptr = reinterpret_cast<const std::uint64_t*>(signal_file.data() + epoch_size_bytes * i);
				auto header = *ptr & ((1 << 24) - 1);
				if (header - last_header != 1)
					throw std::runtime_error("Header ms count mismatch");
				last_header = header;
			}
		}

		void OpenFile() {
			signal_file.open(signal_file_path.string());
			if (!signal_file.is_open())
				throw std::runtime_error("Unable to open file");

			switch (file_type) {
			case FileType::Iq_8_plus_8:
				number_of_epochs = static_cast<std::size_t>(signal_file.size() / (sampling_rate / 1e3) / sizeof(std::complex<std::int8_t>));
				break;
			case FileType::Iq_16_plus_16:
				number_of_epochs = static_cast<std::size_t>(signal_file.size() / (sampling_rate / 1e3) / sizeof(std::complex<std::int16_t>));
				break;
			case FileType::Real_8:
			case FileType::Nt1065GrabberFirst:
			case FileType::Nt1065GrabberSecond:
			case FileType::Nt1065GrabberThird:
			case FileType::Nt1065GrabberFourth:
				number_of_epochs = static_cast<std::size_t>(signal_file.size() / (sampling_rate / 1e3));
				break;
			case FileType::BbpDdc: {
				auto epoch_size_words = sampling_rate / 1e3 * 4 / 64 + 1; // 2+2 samples in 64-bit words plus header
				epoch_size_bytes = static_cast<std::size_t>(epoch_size_words / 2) * 2;
				if (std::fmod(epoch_size_words, 2.0))
					epoch_size_bytes += 2;
				epoch_size_bytes *= 8;
				number_of_epochs = signal_file.size() / epoch_size_bytes;
				VerifyHeaders();

				break;
			}
			default:
				throw std::runtime_error("Unexpected file type");
			}
		}

#ifdef HAS_IPP
		static auto GetConvertWrapper() {
			static auto convert_wrapper = plusifier::FunctionWrapper(
				ippsConvert_8s32f,
				[](const Ipp8s* src, Ipp64f* dst, int len) {
					static thread_local std::vector<Ipp32f> internal_data(len); CheckResize(internal_data, len);
					ippsConvert_8s32f(src, internal_data.data(), len);
					ippsConvert_32f64f(internal_data.data(), dst, len);
				},
				ippsConvert_16s32f,
				[](const Ipp16s* src, Ipp64f* dst, int len) {
					ippsConvert_16s64f_Sfs(src, dst, len, 0);
				},
				ippsCopy_8u
			);
			return convert_wrapper;
		}

		static auto GetRealToComplexWrapper() {
			static auto real_to_complex_wrapper = plusifier::FunctionWrapper(
				ippsRealToCplx_32f,
				ippsRealToCplx_64f
			);
			return real_to_complex_wrapper;
		}
#else
		static auto GetConvertWrapper() {
			return [](auto* src, auto* dst, int size) {
				std::copy(src, src + size, dst);
			};
		}
#endif

#ifdef HAS_IPP
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
#endif

		void GetPartialSignal(std::size_t length_samples, std::size_t samples_offset, OutputVectorType& dst) {
			switch (file_type) {
			case FileType::Iq_8_plus_8: {
				auto ptr_start = signal_file.data() + samples_offset * sizeof(std::complex<std::int8_t>);
				CheckResize(dst, length_samples);

				auto convert_wrapper = GetConvertWrapper();
				convert_wrapper(reinterpret_cast<const int8_t*>(ptr_start), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(dst.size()) * 2);
				break;
			}
			case FileType::Iq_16_plus_16: {
				auto ptr_start = signal_file.data() + samples_offset * sizeof(std::complex<std::int16_t>);
				CheckResize(dst, length_samples);

				auto convert_wrapper = GetConvertWrapper();
				convert_wrapper(reinterpret_cast<const int16_t*>(ptr_start), reinterpret_cast<UnderlyingType*>(dst.data()), static_cast<int>(dst.size()) * 2);
				break;
			}
			case FileType::Real_8: {
				auto ptr_start = signal_file.data() + samples_offset * sizeof(std::int8_t);
				CheckResize(dst, length_samples);

#ifdef HAS_IPP
				static thread_local std::vector<UnderlyingType> local_data(length_samples); CheckResize(local_data, length_samples);
				auto convert_wrapper = GetConvertWrapper();
				convert_wrapper(reinterpret_cast<const int8_t*>(ptr_start), local_data.data(), static_cast<int>(dst.size()));

				auto real_to_complex_wrapper = GetRealToComplexWrapper();
				using IppType = typename IppTypeToComplex<UnderlyingType>::Type;
				real_to_complex_wrapper(local_data.data(), nullptr, reinterpret_cast<IppType*>(dst.data()), static_cast<int>(length_samples));
#else
				std::copy(ptr_start, ptr_start + length_samples, dst.data());
#endif
				break;
			}
#ifdef HAS_IPP
			case FileType::Nt1065GrabberFirst:
			case FileType::Nt1065GrabberSecond:
			case FileType::Nt1065GrabberThird:
			case FileType::Nt1065GrabberFourth: 
				GetPartialSignalNt1065(2 * (static_cast<int>(FileType::Nt1065GrabberFourth) - static_cast<int>(file_type)), length_samples, samples_offset, dst);
				break;
#else
			case FileType::Nt1065GrabberFirst:
			case FileType::Nt1065GrabberSecond:
			case FileType::Nt1065GrabberThird:
			case FileType::Nt1065GrabberFourth: {
				CheckResize(dst, length_samples);
				auto samples_per_ms = static_cast<std::size_t>(sampling_rate / 1e3);
				auto epochs_offset = samples_offset / samples_per_ms;
				auto length_epochs = length_samples / samples_per_ms;

				for (std::size_t i = epochs_offset; i < epochs_offset + length_epochs; ++i) {
					auto ptr = reinterpret_cast<const std::byte*>(signal_file.data() + epoch_size_bytes * i);

					// Unfortunately, case argument isn't considered a compile-time variable
					switch (file_type) {
					case FileType::Nt1065GrabberFirst: {
						auto packed_span = NtlabPackedSpan<6, UnderlyingType>(ptr, samples_per_ms);
						std::copy(std::execution::par_unseq, packed_span.begin(), packed_span.end(), dst.begin() + samples_per_ms * (i - epochs_offset));
						break;
					}
					case FileType::Nt1065GrabberSecond: {
						auto packed_span = NtlabPackedSpan<4, UnderlyingType>(ptr, samples_per_ms);
						std::copy(std::execution::par_unseq, packed_span.begin(), packed_span.end(), dst.begin() + samples_per_ms * (i - epochs_offset));
						break;
					}
					case FileType::Nt1065GrabberThird: {
						auto packed_span = NtlabPackedSpan<2, UnderlyingType>(ptr, samples_per_ms);
						std::copy(std::execution::par_unseq, packed_span.begin(), packed_span.end(), dst.begin() + samples_per_ms * (i - epochs_offset));
						break;
					}
					case FileType::Nt1065GrabberFourth: {
						auto packed_span = NtlabPackedSpan<0, UnderlyingType>(ptr, samples_per_ms);
						std::copy(std::execution::par_unseq, packed_span.begin(), packed_span.end(), dst.begin() + samples_per_ms * (i - epochs_offset));
						break;
					}
					default:
						break;
					}
				}
				break;
			}
#endif
			case FileType::BbpDdc: {
				CheckResize(dst, length_samples);
				auto samples_per_ms = static_cast<std::size_t>(sampling_rate / 1e3);
				auto epochs_offset = samples_offset / samples_per_ms;
				auto length_epochs = length_samples / samples_per_ms;
				auto samples_raw = epoch_size_bytes - 8;
				auto samples_unpacked = epoch_size_bytes * 4;

				for (std::size_t i = epochs_offset; i < epochs_offset + length_epochs; ++i) {
					auto ptr = reinterpret_cast<const std::byte*>(signal_file.data() + epoch_size_bytes * i + 8);

					auto packed_span = PackedSpan<UnderlyingType>(ptr, samples_per_ms);
					std::copy(std::execution::par_unseq, packed_span.begin(), packed_span.end(), dst.begin() + samples_per_ms * (i - epochs_offset));
				}
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
			if (ms_cnt + ms_offset > number_of_epochs)
				throw std::runtime_error("Exceeding epoch requested");
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
