#define NOMINMAX
#include "gtest/gtest.h"

#include "../src/correlator/af_correlator.hpp"
#include "../src/correlator/ipp_correlator.hpp"
#include "../src/prn_codes/GpsL1Ca.hpp"
#include "../src/prn_codes/GlonassOf.hpp"

#include "../src/helpers/af_array_proxy.hpp"
#include "../src/helpers/is_complex.hpp"

#include "../src/matched_filter/ipp_matched_filter.hpp"
#include "../src/matched_filter/af_matched_filter.hpp"

#include "../src/mixer/af_mixer.hpp"
#include "../src/mixer/batch_mixer.hpp"
#include "../src/mixer/ipp_mixer.hpp"
#include "../src/mixer/table_mixer.hpp"

#include "../src/math/ipp_abs.hpp"
#include "../src/math/ipp_conj.hpp"
#include "../src/math/af_dft.hpp"
#include "../src/math/ipp_dft.hpp"

#include "../src/resample/ipp_decimator.hpp"
#include "../src/resample/ipp_resampler.hpp"
#include "../src/resample/ipp_upsampler.hpp"

#include "../src/signal_parameters.hpp"

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
	}

	namespace HelpersTests {
		namespace ArrayProxy {
			template <typename T>
			class ArrayProxyTest : public testing::Test {
			public:
				using Type = T;
			};
			using ArrayProxyTypes = ::testing::Types<
				std::int16_t, std::uint16_t, std::int32_t,
				std::uint32_t, std::int64_t, std::uint64_t, float, double,
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

		TYPED_TEST(MixerTest, ipp_mixer) {
			TestMixer<ugsdr::IppMixer, typename TestFixture::Type>();
		}

		TYPED_TEST(MixerTest, table_mixer) {
			TestMixer<ugsdr::TableMixer, typename TestFixture::Type>();
		}

		TYPED_TEST(MixerTest, af_mixer) {
			TestMixer<ugsdr::AfMixer, typename TestFixture::Type>(1e-3);
		}
	}

	namespace MatchedFilterTests {
		template <typename T>
		class MatchedFilterTest : public testing::Test {
		public:
			using Type = T;
		};
		using MatchedFilterTypes = ::testing::Types<
			ugsdr::SequentialMatchedFilter, ugsdr::IppMatchedFilter, ugsdr::AfMatchedFilter>;
		TYPED_TEST_SUITE(MatchedFilterTest, MatchedFilterTypes);

		TYPED_TEST(MatchedFilterTest, float_matched_filter) {
			const auto signal = ugsdr::Codegen<ugsdr::GlonassOf>::Get<std::complex<float>>(0);
			const auto code = ugsdr::Codegen<ugsdr::GlonassOf>::Get<float>(0);

			auto dst = TestFixture::Type::Filter(signal, code);

			ASSERT_NEAR(dst[0].real(), code.size(), 1e-4);
			for (std::size_t i = 1; i < dst.size(); ++i)
				ASSERT_NEAR(dst[i].real(), -1.0, 1e-4);
		}
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

			TYPED_TEST(AbsTest, ipp_abs) {
				const std::vector<std::complex<typename TestFixture::Type>> data(1000, { 1, -1 });

				auto result = ugsdr::IppAbs::Transform(data);

				ASSERT_EQ(result.size(), data.size());
				for (auto& el : result)
					ASSERT_NEAR(el, std::sqrt(2.0), 1e-2);
			}
		}
		namespace Conj {
			template <typename T>
			class ConjTest : public testing::Test {
			public:
				using Type = T;
			};
			using ConjTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(ConjTest, ConjTypes);

			TYPED_TEST(ConjTest, ipp_conj) {
				const std::vector<std::complex<typename TestFixture::Type>> data(1000, { 1, 1 });

				auto result = ugsdr::IppConj::Transform(data);

				ASSERT_EQ(result.size(), data.size());
				for (auto& el : result) {
					ASSERT_DOUBLE_EQ(el.real(), 1);
					ASSERT_DOUBLE_EQ(el.imag(), -1);
				}
			}
		}
		namespace Dft {
			template <typename T>
			class DftTest : public testing::Test {
			public:
				using Type = T;
			};
			using DftTypes = ::testing::Types<float/*, double*/>;
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
				ASSERT_NEAR(result[result.size() / 10].real(), result.size(), 1e-4);
				ASSERT_NEAR(result[result.size() / 10].imag(), 0, 1e-4);
				ASSERT_NEAR(std::abs(result[result.size() / 10]), result.size(), 1e-4);

			}

			TYPED_TEST(DftTest, ipp_dft) {
				TestPeak<ugsdr::IppDft, typename TestFixture::Type>();
			}

			TYPED_TEST(DftTest, af_dft) {
				TestPeak<ugsdr::AfDft, typename TestFixture::Type>();
			}
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

		TYPED_TEST(ResampleTest, sequential_upsampler) {
			TestUpsampler<ugsdr::SequentialUpsampler, typename TestFixture::Type>();
		}

		TYPED_TEST(ResampleTest, ipp_upsampler) {
			TestUpsampler<ugsdr::IppUpsampler, typename TestFixture::Type>();
		}
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

		TYPED_TEST(SignalParametersTest, iq_8_plus_8) {
			auto sampling_rate = 79.5e6 / 2;
			auto central_frequency = 1590e6;
			auto signal_params = ugsdr::SignalParametersBase<typename TestFixture::Type>(R"(..\..\..\..\data\iq_8_plus_8.bin)",
				ugsdr::FileType::Iq_8_plus_8, central_frequency, sampling_rate);

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

		template <typename T>
		void TestNt1065Grabber(ugsdr::FileType file_type, int value) {
			auto sampling_rate = 79.5e6;
			auto central_frequency = 1590e6;
			auto signal_params = ugsdr::SignalParametersBase<T>(R"(..\..\..\..\data\iq_8_plus_8.bin)",
				file_type, central_frequency, sampling_rate);

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
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFirst, -1);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_second) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberSecond, 1);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_third) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberThird, 3);
		}

		TYPED_TEST(SignalParametersTest, nt1065_grabber_fourth) {
			TestNt1065Grabber<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFourth, -1);
		}
	}
}
