﻿#include "gtest/gtest.h"

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

#include "../src/digital_filter/fir.hpp"
#include "../src/digital_filter/ipp_customized_fir.hpp"
#include "../src/digital_filter/ipp_fir.hpp"

#include "../src/mixer/af_mixer.hpp"
#include "../src/mixer/batch_mixer.hpp"
#include "../src/mixer/ipp_mixer.hpp"
#include "../src/mixer/table_mixer.hpp"

#include "../src/math/af_abs.hpp"
#include "../src/math/af_conj.hpp"
#include "../src/math/ipp_abs.hpp"
#include "../src/math/ipp_conj.hpp"
#include "../src/math/dft.hpp"
#include "../src/math/af_dft.hpp"
#include "../src/math/ipp_dft.hpp"
#include "../src/math/af_max_index.hpp"
#include "../src/math/ipp_max_index.hpp"
#include "../src/math/af_mean_stddev.hpp"
#include "../src/math/ipp_mean_stddev.hpp"
#include "../src/math/af_reshape_and_sum.hpp"
#include "../src/math/ipp_reshape_and_sum.hpp"
#include "../src/math/stft.hpp"
#include "../src/math/ipp_stft.hpp"

#include "../src/resample/decimator.hpp"
#include "../src/resample/af_decimator.hpp"
#include "../src/resample/ipp_decimator.hpp"
#include "../src/resample/af_resampler.hpp"
#include "../src/resample/ipp_resampler.hpp"
#include "../src/resample/ipp_upsampler.hpp"

#include "../src/signal_parameters.hpp"

#include "../src/dfe/dfe.hpp"
#include "../src/acquisition/fse.hpp"

#include "../src/tracking/tracker.hpp"

#include "../src/measurements/measurement_engine.hpp"

#include "../src/positioning/standalone_rtklib.hpp"

#include <random>
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

	namespace DigitalFilterTests {
		namespace Fir {
			template <typename T>
			class FirTest : public testing::Test {
			public:
				using Type = T;
			};
			using FirTypes = ::testing::Types<float, double, std::complex<float>, std::complex<double>>;
			TYPED_TEST_SUITE(FirTest, FirTypes);

			template <typename T>
			auto GetSine(double sine_freq, double fs) {
				std::vector<T> vec(static_cast<std::size_t>(fs / 1e3));
				for (std::size_t i = 0; i < vec.size(); ++i)
					if constexpr (ugsdr::is_complex_v<T>)
						vec[i] = std::exp(T(0, 2 * std::numbers::pi * i * sine_freq / fs));
					else
						vec[i] = static_cast<T>(std::cos(2 * std::numbers::pi * i * sine_freq / fs));

				return vec;
			}

			template <typename T>
			auto GetData(double fs, double freq_low, double freq_high) {
				auto dst = GetSine<T>(freq_low, fs);
				auto harmonic = GetSine<T>(freq_high, fs);
				std::transform(dst.begin(), dst.end(), harmonic.begin(), dst.begin(), std::plus<T>{});

				return dst;
			}

			template <typename T>
			auto GetWeights() {
				std::vector<T> dst(32, static_cast<T>(1.0 / 32 / 0.45022));
				return dst;
			}

			template <typename T, typename F>
			auto Process(const std::vector<T>& data, F& fir) {
				return fir.Filter(data);
			}

			template <template <typename, typename, typename> typename FirType, typename AbsType, typename DftType, typename T>
			void TestFir() {
				auto fs = 50e6;
				auto freq_low = 1e6;
				auto freq_high = 20e6;

				const auto data = GetData<T>(fs, freq_low, freq_high);
				auto weights = GetWeights<T>();
				auto fir = FirType(weights);
				const auto result = Process(data, fir);
				auto pre = AbsType::Transform(DftType::Transform(data));
				auto post = AbsType::Transform(DftType::Transform(result));

				auto low_index = static_cast<std::size_t>(freq_low / 1e3);
				auto high_index = static_cast<std::size_t>(freq_high / 1e3);
				ASSERT_NEAR(post[low_index], pre[low_index], static_cast<ugsdr::underlying_t<T>>(12.0));
				ASSERT_NEAR(post[high_index], pre[high_index] * 0.0428916, static_cast<ugsdr::underlying_t<T>>(1.0));
			}
			template <template <typename, typename, typename> typename FirType, typename AbsType, typename DftType, typename T>
			void TestFirMultiple() {
				auto fs = 50e6;
				auto freq_low = 1e6;
				auto freq_high = 20e6;

				const auto data = GetData<T>(fs, freq_low, freq_high);
				auto weights = GetWeights<T>();
				auto fir = FirType(weights);
				const auto result_first = Process(data, fir);
				const auto result = Process(data, fir);
				auto pre = AbsType::Transform(DftType::Transform(data));
				auto post = AbsType::Transform(DftType::Transform(result));

				auto low_index = static_cast<std::size_t>(freq_low / 1e3);
				auto high_index = static_cast<std::size_t>(freq_high / 1e3);
				ASSERT_NEAR(post[low_index], pre[low_index], static_cast<ugsdr::underlying_t<T>>(12.0));
				ASSERT_NEAR(post[high_index], pre[high_index] * 0.042898, static_cast<ugsdr::underlying_t<T>>(1.0));
			}

#ifdef HAS_IPP
			TYPED_TEST(FirTest, sequential_fir) {
				TestFir<ugsdr::SequentialFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}

			TYPED_TEST(FirTest, sequential_fir_reenter) {
				TestFirMultiple<ugsdr::SequentialFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}
#endif

#ifdef HAS_IPP
			TYPED_TEST(FirTest, ipp_fir) {
				TestFir<ugsdr::IppFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}

			TYPED_TEST(FirTest, ipp_fir_reenter) {
				TestFirMultiple<ugsdr::IppFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}

			TYPED_TEST(FirTest, ipp_customized_fir) {
				TestFir<ugsdr::IppCustomizedFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}

			TYPED_TEST(FirTest, ipp_customized_fir_reenter) {
				TestFirMultiple<ugsdr::IppCustomizedFir, ugsdr::IppAbs, ugsdr::IppDft, typename TestFixture::Type>();
			}
#endif

//#ifdef HAS_ARRAYFIRE
//			TYPED_TEST(FirTest, af_fir) {
//				TestFir<ugsdr::AfFir, typename TestFixture::Type>();
//			}
//#endif
		}
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

				auto result = static_cast<std::vector<T>>(AbsType::Transform(data));

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

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(AbsTest, af_abs) {
				TestAbs<ugsdr::AfAbs, typename TestFixture::Type>();
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

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(ConjTest, af_conj) {
				const std::vector<std::complex<typename TestFixture::Type>> data(1000, { 1, 1 });

				auto result = static_cast<std::vector<std::complex<typename TestFixture::Type>>>(ugsdr::AfConj::Transform(data));

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
		namespace MaxIndex {
			template <typename T>
			class MaxIndexTest : public testing::Test {
			public:
				using Type = T;
			};
			using MaxIndexTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(MaxIndexTest, MaxIndexTypes);

			template <typename MaxIndexType, typename T>
			void TestMaxIndex() {
				std::vector<T> data(1000);
				std::iota(data.begin(), data.end(), static_cast<T>(0));

				auto result = MaxIndexType::Transform(data);

				ASSERT_EQ(result.value, data.size() - 1);
				ASSERT_EQ(result.index, data.size() - 1);
			}

			TYPED_TEST(MaxIndexTest, sequential_max_index) {
				TestMaxIndex<ugsdr::SequentialMaxIndex, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(MaxIndexTest, ipp_max_index) {
				TestMaxIndex<ugsdr::IppMaxIndex, typename TestFixture::Type>();
			}
#endif

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(MaxIndexTest, af_max_index) {
				TestMaxIndex<ugsdr::AfMaxIndex, typename TestFixture::Type>();
			}
#endif
		}
		namespace MeanStdDev {
			template <typename T>
			class MeanStdDevTest : public testing::Test {
			public:
				using Type = T;
			};
			using MeanStdDevTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(MeanStdDevTest, MeanStdDevTypes);

			template <typename MeanStdDevType, typename T>
			void TestMeanStdDev() {
				std::vector<T> data(1000000);
				std::random_device rd;
				std::mt19937 mt(rd());

				auto mean = static_cast<T>(0);
				auto sigma = static_cast<T>(1);
				auto dist = std::normal_distribution<T>(mean, sigma);
				for (auto& el : data)
					el = dist(mt);

				auto result = MeanStdDevType::Calculate(data);

				ASSERT_NEAR(static_cast<T>(result.mean), mean, 0.01);
				ASSERT_NEAR(static_cast<T>(result.sigma), sigma, 0.01);
			}

			TYPED_TEST(MeanStdDevTest, sequential_max_index) {
				TestMeanStdDev<ugsdr::SequentialMeanStdDev, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(MeanStdDevTest, ipp_max_index) {
				TestMeanStdDev<ugsdr::IppMeanStdDev, typename TestFixture::Type>();
			}
#endif

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(MeanStdDevTest, af_max_index) {
				TestMeanStdDev<ugsdr::AfMeanStdDev, typename TestFixture::Type>();
			}
#endif
		}
		namespace ReshapeAndSum {
			template <typename T>
			class ReshapeAndSumTest : public testing::Test {
			public:
				using Type = T;
			};
			using ReshapeAndSumTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(ReshapeAndSumTest, ReshapeAndSumTypes);

			template <typename ReshapeAndSumType, typename T>
			void TestReshapeAndSum() {
				const std::vector<T> data(10000, static_cast<T>(1));
				auto result = static_cast<std::vector<T>>(ReshapeAndSumType::Transform(data, 1000));

				ASSERT_EQ(result.size(), 1000);
				ASSERT_TRUE(std::all_of(result.begin(), result.end(), [](auto& val) { return val == static_cast<T>(10); }));
			}

			TYPED_TEST(ReshapeAndSumTest, sequential_max_index) {
				TestReshapeAndSum<ugsdr::SequentialReshapeAndSum, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(ReshapeAndSumTest, ipp_max_index) {
				TestReshapeAndSum<ugsdr::IppReshapeAndSum, typename TestFixture::Type>();
			}
#endif

#ifdef HAS_ARRAYFIRE
			TYPED_TEST(ReshapeAndSumTest, af_max_index) {
				TestReshapeAndSum<ugsdr::AfReshapeAndSum, typename TestFixture::Type>();
			}
#endif
		}

		namespace Stft {
			template <typename T>
			class StftTest : public testing::Test {
			public:
				using Type = T;
			};
			using StftTypes = ::testing::Types<float, double>;
			TYPED_TEST_SUITE(StftTest, StftTypes);

			template <typename T>
			auto GetSine(double sine_freq, double fs) {
				std::vector<std::complex<T>> vec(static_cast<std::size_t>(fs / 1e3));
				for (std::size_t i = 0; i < vec.size(); ++i)
					vec[i] = std::exp(std::complex<T>(0, 2 * std::numbers::pi * i * sine_freq / fs));

				return vec;
			}

			template <typename T>
			auto GetChirp(double chirp_start, double chirp_end, double chirp_period, double fs) {
				std::vector<std::complex<T>> vec(static_cast<std::size_t>(fs / 1e3));
				auto chirpiness = (chirp_end - chirp_start) / chirp_period;
				auto samples_per_chirp = static_cast<std::size_t>(ceil(chirp_period * fs));
				for (std::size_t i = 0; i < vec.size(); ++i) {
					auto current_time = (i % samples_per_chirp) / fs;
					vec[i] = std::exp(std::complex<T>(0, 2 * std::numbers::pi * (chirpiness * current_time * current_time / 2.0 + chirp_start * current_time)));
				}

				return vec;
			}

			template <typename StftType, typename AbsType, typename MaxIndexType, typename T>
			void TestPeak() {
				auto sampling_rate = 50e6;
				auto tone_freq = 1e6;
				const auto data = GetSine<T>(tone_freq, sampling_rate);
				auto result = StftType::Transform(data);

				std::map<double, std::atomic<double>> freq_to_percent_map;

				std::for_each(result.begin(), result.end(), [&freq_to_percent_map, &sampling_rate, &result](auto& el) {
					auto max_pos = MaxIndexType::Transform(AbsType::Transform(el));
					freq_to_percent_map[max_pos.index * sampling_rate / el.size()] += 100.0 / result.size();
				});

				auto max_freq_offset = sampling_rate / result[0].size() / 2;

				bool peak_found = false;
				for (auto& [frequency, bins] : freq_to_percent_map) {
					if (std::abs(frequency - tone_freq) <= max_freq_offset) {
						ASSERT_GE(bins, 95.0);
						peak_found = true;
					}
					else
						ASSERT_LT(bins, 1.0);
				}
				ASSERT_TRUE(peak_found);
			}

			template <typename StftType, typename AbsType, typename MaxIndexType, typename T>
			void TestChirp() {
				auto sampling_rate = 50e6;
				auto chirp_start = 1e6;
				auto chirp_end = 10e6;
				auto chirp_period = 20e-6;
				const auto data = GetChirp<T>(chirp_start, chirp_end, chirp_period, sampling_rate);
				auto result = StftType::Transform(data);

				std::map<double, std::atomic<double>> freq_to_percent_map;

				std::for_each(result.begin(), result.end(), [&freq_to_percent_map, &sampling_rate, &result](auto& el) {
					auto max_pos = MaxIndexType::Transform(AbsType::Transform(el));
					freq_to_percent_map[max_pos.index * sampling_rate / el.size()] += 100.0 / result.size();
				});

				auto number_of_bins = result[0].size();
				auto frequency_step = sampling_rate / static_cast<double>(number_of_bins);
				auto max_freq_offset = frequency_step / 2;
				auto occupied_bins = (chirp_end - chirp_start) / frequency_step;

				for (auto& [frequency, bins] : freq_to_percent_map) {
					auto delta_start = std::abs(frequency - chirp_start);
					auto delta_end = std::abs(frequency - chirp_end);
					if ((frequency - max_freq_offset >= chirp_start) || (frequency - max_freq_offset <= chirp_end))
						ASSERT_GE(bins, 100.0 / occupied_bins / 2);
					else
						ASSERT_LT(bins, 1.0);
				}
			}

			template <typename StftType, typename AbsType, typename MaxIndexType, typename T>
			void Test() {
				TestPeak<StftType, AbsType, MaxIndexType, T>();
				TestChirp<StftType, AbsType, MaxIndexType, T>();
			}

			TYPED_TEST(StftTest, sequential_stft) {
				Test<ugsdr::SequentialStft, ugsdr::SequentialAbs, ugsdr::SequentialMaxIndex, typename TestFixture::Type>();
			}

#ifdef HAS_IPP
			TYPED_TEST(StftTest, ipp_stft) {
				Test<ugsdr::IppStft, ugsdr::IppAbs, ugsdr::IppMaxIndex, typename TestFixture::Type>();
			}
#endif
//
//#ifdef HAS_ARRAYFIRE
//			TYPED_TEST(StftTest, af_stft) {
//				TestPeak<ugsdr::AfStft, typename TestFixture::Type>();
//			}
//#endif
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

		template <typename AccumulatorImpl, typename T>
		void TestAccumulator() {
			const auto data = GetSlope<T>();

			auto decimation_ratio = 10;
			auto result = AccumulatorImpl::Transform(data, decimation_ratio);

			for (std::size_t i = 0; i < result.size(); ++i) {
				auto val = (data[i * decimation_ratio] + data[(i + 1) * decimation_ratio - 1]) /
					static_cast<ugsdr::underlying_t<T>>(2.0);
				if constexpr (ugsdr::is_complex_v<T>) {
					ASSERT_NEAR(result[i].real(), val.real(), 1e-4);
					ASSERT_NEAR(result[i].imag(), val.imag(), 1e-4);
				}
				else
					ASSERT_NEAR(result[i], val, 1e-4);
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
			TestAccumulator<ugsdr::IppAccumulator, typename TestFixture::Type>();
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

#ifdef HAS_ARRAYFIRE
		TYPED_TEST(ResampleTest, af_decimator) {
			TestDecimator<ugsdr::AfDecimator, typename TestFixture::Type>();
		}
		TYPED_TEST(ResampleTest, af_accumulator) {
			TestAccumulator<ugsdr::AfAccumulator, typename TestFixture::Type>();
		}
#endif
	}
}
