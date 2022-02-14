#include "gtest/gtest.h"

#include "../src/correlator/correlator.hpp"
#include "../src/correlator/af_correlator.hpp"
#include "../src/correlator/ipp_correlator.hpp"
#include "../src/prn_codes/GpsL1Ca.hpp"
#include "../src/prn_codes/GlonassOf.hpp"

#include "../src/helpers/af_array_proxy.hpp"
#include "../src/helpers/is_complex.hpp"

#include "../src/matched_filter/matched_filter.hpp"
#include "../src/matched_filter/ipp_matched_filter.hpp"
#include "../src/matched_filter/af_matched_filter.hpp"

#include "../src/mixer/af_mixer.hpp"
#include "../src/mixer/batch_mixer.hpp"
#include "../src/mixer/ipp_mixer.hpp"
#include "../src/mixer/table_mixer.hpp"

#include "../src/math/ipp_abs.hpp"
#include "../src/math/ipp_conj.hpp"
#include "../src/math/dft.hpp"
#include "../src/math/af_dft.hpp"
#include "../src/math/ipp_dft.hpp"

#include "../src/resample/decimator.hpp"
#include "../src/resample/ipp_decimator.hpp"
#include "../src/resample/ipp_resampler.hpp"
#include "../src/resample/ipp_upsampler.hpp"

#include "../src/signal_parameters.hpp"

#include "../src/dfe/dfe.hpp"
#include "../src/acquisition/fse.hpp"

#include "../src/tracking/tracker.hpp"

#include "../src/measurements/measurement_engine.hpp"

#include "../src/positioning/standalone_rtklib.hpp"

#include <type_traits>

namespace basic_tests {
	namespace CorrelatorTests {
		template <typename T>
		class CorrelatorTest : public testing::Test {
		public:
			using Type = T;
		};
		using CorrelatorTypes = ::testing::Types<float, double>;
		TYPED_TEST_SUITE(CorrelatorTest, CorrelatorTypes);

		TYPED_TEST(CorrelatorTest, sequential_correlator) {
			const auto code = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<typename TestFixture::Type>(0);
			const auto signal = std::vector<std::complex<typename TestFixture::Type>>(code.begin(), code.end());

			auto dst = ugsdr::SequentialCorrelator::Correlate(std::span(signal), std::span(code));

			if (std::is_floating_point_v<typename TestFixture::Type>) {
				ASSERT_DOUBLE_EQ(dst.real(), static_cast<typename TestFixture::Type>(code.size()));
				ASSERT_DOUBLE_EQ(dst.imag(), 0);
			}
			else {
				ASSERT_EQ(dst.real(), code.size());
				ASSERT_EQ(dst.imag(), 0);
			}
		}

#ifdef HAS_IPP
		TYPED_TEST(CorrelatorTest, ipp_correlator) {
			const auto code = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<typename TestFixture::Type>(0);
			const auto signal = std::vector<std::complex<typename TestFixture::Type>>(code.begin(), code.end());

			auto dst = ugsdr::IppCorrelator::Correlate(std::span(signal), std::span(code));

			if (std::is_floating_point_v<typename TestFixture::Type>) {
				ASSERT_DOUBLE_EQ(dst.real(), static_cast<typename TestFixture::Type>(code.size()));
				ASSERT_DOUBLE_EQ(dst.imag(), 0);
			}
			else {
				ASSERT_EQ(dst.real(), code.size());
				ASSERT_EQ(dst.imag(), 0);
			}
		}
#endif

#ifdef HAS_ARRAYFIRE
		TYPED_TEST(CorrelatorTest, af_correlator) {
			const auto signal_real = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<typename TestFixture::Type>(0);
			const auto signal = std::vector<std::complex<typename TestFixture::Type>>(signal_real.begin(), signal_real.end());
			const auto code = signal;

			auto dst = ugsdr::AfCorrelator::Correlate(ugsdr::ArrayProxy(signal), ugsdr::ArrayProxy(code));

			if (std::is_floating_point_v<typename TestFixture::Type>) {
				ASSERT_DOUBLE_EQ(dst.real(), static_cast<typename TestFixture::Type>(code.size()));
				ASSERT_DOUBLE_EQ(dst.imag(), 0);
			}
			else {
				ASSERT_EQ(dst.real(), code.size());
				ASSERT_EQ(dst.imag(), 0);
			}
		}
#endif
	}

	namespace HelpersTests {
#ifdef HAS_ARRAYFIRE
		namespace ArrayProxy {
			template <typename T>
			class ArrayProxyTest : public testing::Test {
			public:
				using Type = T;
			};
			using ArrayProxyTypes = ::testing::Types<
				std::int16_t, std::int32_t, float, double,
				std::complex<float>, std::complex<double>>;
			TYPED_TEST_SUITE(ArrayProxyTest, ArrayProxyTypes);


			TYPED_TEST(ArrayProxyTest, array_proxy_construct) {
				const std::vector<typename TestFixture::Type> data(1000, 1);

				auto proxy = ugsdr::ArrayProxy(data);
				ASSERT_EQ(proxy.size(), data.size());
			}
			TYPED_TEST(ArrayProxyTest, array_proxy_gather) {
				const std::vector<typename TestFixture::Type> data(1000, 1);

				auto proxy = ugsdr::ArrayProxy(data);

				std::vector<typename TestFixture::Type> gathered_data;
				auto optional_vector = proxy.CopyFromGpu(gathered_data);
				ASSERT_FALSE(optional_vector);
				ASSERT_EQ(gathered_data.size(), data.size());
				ASSERT_EQ(gathered_data, data);
			}
			TYPED_TEST(ArrayProxyTest, array_proxy_cast) {
				const std::vector<typename TestFixture::Type> data(1000, 1);

				auto proxy = ugsdr::ArrayProxy(data);

				auto casted_data = static_cast<std::vector<typename TestFixture::Type>>(proxy);
				ASSERT_EQ(casted_data.size(), data.size());
				ASSERT_EQ(casted_data, data);
			}
		}
#endif

		namespace IsComplex {
			template <typename T>
			class IsComplexTest : public testing::Test {
			public:
				using Type = T;
			};
			using IsComplexTypes = ::testing::Types<
				std::int16_t, std::uint16_t, std::int32_t,
				std::uint32_t, std::int64_t, std::uint64_t, float, double>;
			TYPED_TEST_SUITE(IsComplexTest, IsComplexTypes);


			TYPED_TEST(IsComplexTest, real_types) {
				ASSERT_FALSE(ugsdr::is_complex_v<typename TestFixture::Type>);
			}
			TYPED_TEST(IsComplexTest, complex_types) {
				ASSERT_TRUE(ugsdr::is_complex_v<std::complex<typename TestFixture::Type>>);
			}
			TYPED_TEST(IsComplexTest, underlying_types) {
				auto same_type = std::is_same_v<ugsdr::underlying_t<std::complex<typename TestFixture::Type>>,
					typename TestFixture::Type>;

				ASSERT_TRUE(same_type);
			}
		}
	}

	namespace MixerTests {
		template <typename T>
		class MixerTest : public testing::Test {
		public:
			using Type = T;
		};
		using MixerTypes = ::testing::Types</*int, */ float, double>;
		TYPED_TEST_SUITE(MixerTest, MixerTypes);

		template <typename MixerType, typename T>
		void TestMixer(double precision = 1e-12) {
			auto mixer = MixerType(1e6, 1e6 / 2, 0);

			auto signal = std::vector<std::complex<T>>(1000, 1);
			mixer.Translate(signal);

			for (std::size_t i = 0; i < signal.size(); ++i) {
				ASSERT_DOUBLE_EQ(signal[i].real(), 1.0 + -2.0 * (i & 1));
				ASSERT_NEAR(signal[i].imag(), 0, precision);
			}
		}

		TYPED_TEST(MixerTest, sequential_mixer) {
			TestMixer<ugsdr::SequentialMixer, typename TestFixture::Type>();
		}

		TYPED_TEST(MixerTest, batch_mixer) {
			TestMixer<ugsdr::BatchMixer, typename TestFixture::Type>(1e-3);
		}

#ifdef HAS_IPP
		TYPED_TEST(MixerTest, ipp_mixer) {
			TestMixer<ugsdr::IppMixer, typename TestFixture::Type>();
		}
#endif

		TYPED_TEST(MixerTest, table_mixer) {
			TestMixer<ugsdr::TableMixer, typename TestFixture::Type>();
		}

#ifdef HAS_ARRAYFIRE
		TYPED_TEST(MixerTest, af_mixer) {
			TestMixer<ugsdr::AfMixer, typename TestFixture::Type>(1e-3);
		}
#endif
	}

	namespace MatchedFilterTests {
		template <typename T>
		class MatchedFilterTest : public testing::Test {
		public:
			using Type = T;
		};
		using MatchedFilterTypes = ::testing::Types<float, double>;
		TYPED_TEST_SUITE(MatchedFilterTest, MatchedFilterTypes);

		template <typename FilterType, typename T>
		void TestMatched() {
			const auto signal = ugsdr::Codegen<ugsdr::GlonassOf>::Get<std::complex<T>>(0);
			const auto code = ugsdr::Codegen<ugsdr::GlonassOf>::Get<T>(0);

			auto dst = FilterType::Filter(signal, code);

			ASSERT_NEAR(dst[0].real(), code.size(), 5e-3);
			for (std::size_t i = 1; i < dst.size(); ++i)
				ASSERT_NEAR(dst[i].real(), -1.0, 5e-3);
			
		}
		
		TYPED_TEST(MatchedFilterTest, sequential_matched_filter) {
			TestMatched<ugsdr::SequentialMatchedFilter, typename TestFixture::Type>();
		}

#ifdef HAS_IPP
		TYPED_TEST(MatchedFilterTest, ipp_matched_filter) {
			TestMatched<ugsdr::IppMatchedFilter, typename TestFixture::Type>();
		}
#endif

#ifdef HAS_ARRAYFIRE
		TYPED_TEST(MatchedFilterTest, af_matched_filter) {
			TestMatched<ugsdr::AfMatchedFilter, typename TestFixture::Type>();
		}
#endif
	}

	namespace MathTests {
		namespace Abs {
			template <typename T>
			class AbsTest : public testing::Test {
			public:
				using Type = T;
			};
			using AbsTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(AbsTest, AbsTypes);

			template <typename AbsType, typename T>
			void TestAbs() {
				const std::vector<std::complex<T>> data(1000, { 1, -1 });

				auto result = AbsType::Transform(data);

				ASSERT_EQ(result.size(), data.size());
				for (auto& el : result)
					ASSERT_NEAR(el, std::sqrt(2.0), 1e-2);
			}

			TYPED_TEST(AbsTest, sequential_abs) {
				TestAbs<ugsdr::SequentialAbs, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(AbsTest, ipp_abs) {
				TestAbs<ugsdr::IppAbs, typename TestFixture::Type>();
			}
#endif
		}
		namespace Conj {
			template <typename T>
			class ConjTest : public testing::Test {
			public:
				using Type = T;
			};
			using ConjTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(ConjTest, ConjTypes);

#ifdef HAS_IPP
			TYPED_TEST(ConjTest, ipp_conj) {
				const std::vector<std::complex<typename TestFixture::Type>> data(1000, { 1, 1 });

				auto result = ugsdr::IppConj::Transform(data);

				ASSERT_EQ(result.size(), data.size());
				for (auto& el : result) {
					ASSERT_DOUBLE_EQ(el.real(), 1);
					ASSERT_DOUBLE_EQ(el.imag(), -1);
				}
			}
#endif
		}

		namespace Dft {
			template <typename T>
			class DftTest : public testing::Test {
			public:
				using Type = T;
			};
			using DftTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(DftTest, DftTypes);

			template <typename T>
			auto GetVector() {
				std::vector<std::complex<T>> vec(1000);
				for (std::size_t i = 0; i < vec.size(); ++i)
					vec[i] = std::exp(std::complex<T>(0, 2 * std::numbers::pi * i * 0.1));

				return vec;
			}

			template <typename DftType, typename T>
			void TestPeak() {
				const auto data = GetVector<T>();
				auto result = static_cast<std::vector<std::complex<T>>>(DftType::Transform(data));

				ASSERT_EQ(result.size(), data.size());
				ASSERT_NEAR(result[result.size() / 10].real(), result.size(), 5e-3);
				ASSERT_NEAR(result[result.size() / 10].imag(), 0, 5e-3);
				ASSERT_NEAR(std::abs(result[result.size() / 10]), result.size(), 5e-3);

			}

			TYPED_TEST(DftTest, sequential_dft) {
				TestPeak<ugsdr::SequentialDft, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(DftTest, ipp_dft) {
				TestPeak<ugsdr::IppDft, typename TestFixture::Type>();
			}
#endif

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(DftTest, af_dft) {
				TestPeak<ugsdr::AfDft, typename TestFixture::Type>();
			}
#endif
		}
	}

	namespace ResampleTests {
		template <typename T>
		class ResampleTest : public testing::Test {
		public:
			using Type = T;
		};
		using ResampleTypes = ::testing::Types<float, double, std::complex<float>, std::complex<double>>;
		TYPED_TEST_SUITE(ResampleTest, ResampleTypes);

		template <typename T>
		auto GetSlope() {
			std::vector<T> vec(1000);
			for (std::size_t i = 0; i < vec.size(); ++i)
				vec[i] = i;
			return vec;
		}

		template <typename DecimatorImpl, typename T>
		void TestDecimator() {
			const auto data = GetSlope<T>();

			auto decimation_ratio = 10;
			auto result = DecimatorImpl::Transform(data, decimation_ratio);

			for (std::size_t i = 0; i < result.size(); ++i) {
				if constexpr (ugsdr::is_complex_v<T>) {
					ASSERT_DOUBLE_EQ(result[i].real(), data[i * decimation_ratio].real());
					ASSERT_DOUBLE_EQ(result[i].imag(), data[i * decimation_ratio].imag());
				}
				else
					ASSERT_DOUBLE_EQ(result[i], data[i * decimation_ratio]);
			}
		}

		template <typename UpsamplerImpl, typename T>
		void TestUpsampler() {
			const auto data = GetSlope<T>();

			auto up_ratio = 10;
			auto result = UpsamplerImpl::Transform(data, data.size() * up_ratio);

			for (std::size_t i = 0; i < data.size(); ++i) {
				for (std::size_t j = 0; j < up_ratio; ++j) {
					if constexpr (ugsdr::is_complex_v<T>) {
						ASSERT_NEAR(result[i * up_ratio + j].real(), data[i].real(), 1e-4);
						ASSERT_NEAR(result[i * up_ratio + j].imag(), data[i].imag(), 1e-4);
					}
					else
						ASSERT_NEAR(result[i * up_ratio + j], data[i], 1e-4);
				}
			}
		}

		TYPED_TEST(ResampleTest, sequential_decimator) {
			TestDecimator<ugsdr::SequentialDecimator, typename TestFixture::Type>();
		}

#ifdef HAS_IPP
		TYPED_TEST(ResampleTest, ipp_decimator) {
			TestDecimator<ugsdr::IppDecimator, typename TestFixture::Type>();
		}

		TYPED_TEST(ResampleTest, ipp_accumulator) {
			const auto data = GetSlope<typename TestFixture::Type>();

			auto decimation_ratio = 10;
			auto result = ugsdr::IppAccumulator::Transform(data, decimation_ratio);

			for (std::size_t i = 0; i < result.size(); ++i) {
				auto val = (data[i * decimation_ratio] + data[(i + 1) * decimation_ratio - 1]) /
					static_cast<ugsdr::underlying_t<typename TestFixture::Type>>(2.0);
				if constexpr (ugsdr::is_complex_v<typename TestFixture::Type>) {
					ASSERT_NEAR(result[i].real(), val.real(), 1e-4);
					ASSERT_NEAR(result[i].imag(), val.imag(), 1e-4);
				}
				else
					ASSERT_NEAR(result[i], val, 1e-4);
			}
		}
#endif

		TYPED_TEST(ResampleTest, sequential_upsampler) {
			TestUpsampler<ugsdr::SequentialUpsampler, typename TestFixture::Type>();
		}

#ifdef HAS_IPP
		TYPED_TEST(ResampleTest, ipp_upsampler) {
			TestUpsampler<ugsdr::IppUpsampler, typename TestFixture::Type>();
		}
#endif
	}
}

namespace integration_tests {
	namespace SignalParametersTests {
		template <typename T>
		class SignalParametersTest : public testing::Test {
		public:
			using Type = T;
		};
		using SignalParametersTypes = ::testing::Types<float, double>;
		TYPED_TEST_SUITE(SignalParametersTest, SignalParametersTypes);

		template <typename T>
		auto GetSignalParameters(ugsdr::FileType file_type) {
			std::map<ugsdr::FileType, ugsdr::SignalParametersBase<T>> map{
				{ ugsdr::FileType::Iq_8_plus_8 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("iq_8_plus_8.bin"), ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2) },
				{ ugsdr::FileType::Iq_16_plus_16 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("iq_16_plus_16.bin"), ugsdr::FileType::Iq_16_plus_16, 1575.42e6, 4e6) },
				{ ugsdr::FileType::Real_8 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("real_8.bin"), ugsdr::FileType::Real_8, 1575.42e6 + 9.55e6, 38.192e6) },
				{ ugsdr::FileType::Nt1065GrabberFirst , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberSecond , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberThird , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberThird, 1200e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberFourth , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberFourth, 1200e6, 79.5e6) },
			};

			return map.at(file_type);
		}
		
		TYPED_TEST(SignalParametersTest, iq_8_plus_8) {
			auto sampling_rate = 79.5e6 / 2;
			auto central_frequency = 1590e6;
			auto signal_params = GetSignalParameters<typename TestFixture::Type>(ugsdr::FileType::Iq_8_plus_8);

			ASSERT_EQ(signal_params.GetSamplingRate(), sampling_rate);
			ASSERT_EQ(signal_params.GetCentralFrequency(), central_frequency);
			ASSERT_EQ(signal_params.GetNumberOfEpochs(), 100);

			auto single_ms = signal_params.GetOneMs(0);
			ASSERT_EQ(single_ms.size(), static_cast<std::size_t>(sampling_rate / 1e3));
			ASSERT_EQ(single_ms[0], std::complex<typename TestFixture::Type>(28, -2));
			try {
				auto exceeding_epoch = signal_params.GetOneMs(signal_params.GetNumberOfEpochs() + 1);
			}
			catch (...) { return; }
			FAIL();
		}

		TYPED_TEST(SignalParametersTest, iq_16_plus_16) {
			auto sampling_rate = 4e6;
			auto central_frequency = 1575.42e6;
			auto signal_params = GetSignalParameters<typename TestFixture::Type>(ugsdr::FileType::Iq_16_plus_16);

			ASSERT_EQ(signal_params.GetSamplingRate(), sampling_rate);
			ASSERT_EQ(signal_params.GetCentralFrequency(), central_frequency);
			ASSERT_EQ(signal_params.GetNumberOfEpochs(), 100);

			auto single_ms = signal_params.GetOneMs(0);
			ASSERT_EQ(single_ms.size(), static_cast<std::size_t>(sampling_rate / 1e3));
			ASSERT_EQ(single_ms[0], std::complex<typename TestFixture::Type>(-150, 16));
			try {
				auto exceeding_epoch = signal_params.GetOneMs(signal_params.GetNumberOfEpochs() + 1);
			}
			catch (...) { return; }
			FAIL();
		}

		TYPED_TEST(SignalParametersTest, real_8) {
			auto sampling_rate = 38.192e6;
			auto central_frequency = 1575.42e6 + 9.55e6; 
			auto signal_params = GetSignalParameters<typename TestFixture::Type>(ugsdr::FileType::Real_8);

			ASSERT_EQ(signal_params.GetSamplingRate(), sampling_rate);
			ASSERT_EQ(signal_params.GetCentralFrequency(), central_frequency);
			ASSERT_EQ(signal_params.GetNumberOfEpochs(), 100);

			auto single_ms = signal_params.GetOneMs(0);
			ASSERT_EQ(single_ms.size(), static_cast<std::size_t>(sampling_rate / 1e3));
			ASSERT_EQ(single_ms[0], std::complex<typename TestFixture::Type>(-2));
			try {
				auto exceeding_epoch = signal_params.GetOneMs(signal_params.GetNumberOfEpochs() + 1);
			}
			catch (...) { return; }
			FAIL();
		}

		template <typename T>
		void TestNt1065Grabber(ugsdr::FileType file_type, double central_frequency, int value) {
			auto sampling_rate = 79.5e6;
			auto signal_params = GetSignalParameters<T>(file_type);

			ASSERT_EQ(signal_params.GetSamplingRate(), sampling_rate);
			ASSERT_EQ(signal_params.GetCentralFrequency(), central_frequency);
			ASSERT_EQ(signal_params.GetNumberOfEpochs(), 100);

			auto single_ms = signal_params.GetOneMs(0);
			ASSERT_EQ(single_ms.size(), static_cast<std::size_t>(sampling_rate / 1e3));
			ASSERT_EQ(single_ms[0], std::complex<T>(value));
			try {
				auto exceeding_epoch = signal_params.GetOneMs(signal_params.GetNumberOfEpochs() + 1);
			}
			catch (...) { return; }
			FAIL();			
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_first) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 1);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_second) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 1);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_third) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberThird, 1200e6, 1);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_fourth) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFourth, 1200e6, 3);
		}
	}

	namespace AcquisitionTests {
		template <typename T>
		class AcquisitionTest : public testing::Test {
		public:
			using Type = T;
		};
		using AcquisitionTypes = ::testing::Types<float, double>;
		TYPED_TEST_SUITE(AcquisitionTest, AcquisitionTypes);

		template <typename T>
		auto GetSignalParameters(ugsdr::FileType file_type) {
			std::map<ugsdr::FileType, ugsdr::SignalParametersBase<T>> map{
				{ ugsdr::FileType::Iq_8_plus_8 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("iq_8_plus_8.bin"), ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2) },
				{ ugsdr::FileType::Iq_16_plus_16 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("iq_16_plus_16.bin"), ugsdr::FileType::Iq_16_plus_16, 1575.42e6, 4e6) },
				{ ugsdr::FileType::Real_8 , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("real_8.bin"), ugsdr::FileType::Real_8, 1575.42e6 + 9.55e6, 38.192e6) },
				{ ugsdr::FileType::Nt1065GrabberFirst , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberSecond , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberThird , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberThird, 1200e6, 79.5e6) },
				{ ugsdr::FileType::Nt1065GrabberFourth , ugsdr::SignalParametersBase<T>(SIGNAL_DATA_PATH + std::string("nt1065_grabber.bin"), ugsdr::FileType::Nt1065GrabberFourth, 1200e6, 79.5e6) },
			};

			return map.at(file_type);
		}

		template <typename T>
		void TestAcquisition(ugsdr::FileType file_type, ugsdr::Signal signal, double doppler_range = 5e3) {
			auto signal_parameters = GetSignalParameters<T>(file_type);

			auto digital_frontend = ugsdr::DigitalFrontend(
				MakeChannel(signal_parameters, signal, signal_parameters.GetSamplingRate())
			);

			auto fse = ugsdr::FastSearchEngineBase(digital_frontend, doppler_range, 200);
			auto acquisition_results = fse.Process(false);

			ASSERT_FALSE(acquisition_results.empty());
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gln) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_16_plus_16_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_16_plus_16, ugsdr::Signal::GpsCoarseAcquisition_L1, 15e3);
		}

		TYPED_TEST(AcquisitionTest, real_8_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Real_8, ugsdr::Signal::GpsCoarseAcquisition_L1, 6e3);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFirst, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gln) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberSecond, ugsdr::Signal::GlonassCivilFdma_L1);
		}
	}

	namespace PositioningTests {
		template <typename T>
		class PositioningTest : public testing::Test {
		public:
			using Type = T;
		};
		using PositioningTypes = ::testing::Types<float, double>;
		TYPED_TEST_SUITE(PositioningTest, PositioningTypes);

		
		TYPED_TEST(PositioningTest, GPSdata_DiscreteComponents) {
			auto signal_parameters = ugsdr::SignalParametersBase<typename TestFixture::Type>(SIGNAL_DATA_PATH +
				std::string("GPSdata-DiscreteComponents-fs38_192-if9_55.bin"), 
				ugsdr::FileType::Real_8, 1575.42e6 + 9.55e6, 38.192e6);
			auto digital_frontend = ugsdr::DigitalFrontend(
				MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::GpsCoarseAcquisition_L1 }, 
					signal_parameters.GetSamplingRate() / 4)
			);
			auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 6e3, 200);
			auto acquisition_results = fse.Process();
			ASSERT_FALSE(acquisition_results.empty());
			auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
			tracker.Track(signal_parameters.GetNumberOfEpochs());

			auto measurement_engine = ugsdr::MeasurementEngine(tracker.GetTrackingParameters());
			auto positioning_engine = ugsdr::StandaloneRtklib(measurement_engine);
			auto reference_position = std::vector{ -1288161.849718, -4720800.361224, 4079714.93957036 };	// matlab reference
			
			auto pos_and_time = positioning_engine.EstimatePosition(signal_parameters.GetNumberOfEpochs() - 1);
			auto pos = std::vector{ std::get<0>(pos_and_time), std::get<1>(pos_and_time), std::get<2>(pos_and_time)};

			for (std::size_t i = 0; i < pos.size(); ++i)
				pos[i] -= reference_position[i];
			auto offset = std::sqrt(std::accumulate(pos.begin(), pos.end(), 0.0, [](const auto& sum, const auto& val) {
				return sum + val * val;
			}));
			std::cout << "Delta: " << offset << "meters" << std::endl;
			ASSERT_LE(offset, 10);
		}
	}

}
