#include "gtest/gtest.h"

#include "../src/correlator/af_correlator.hpp"
#include "../src/correlator/ipp_correlator.hpp"
#include "../src/prn_codes/GpsL1Ca.hpp"
#include "../src/prn_codes/GlonassOf.hpp"

#include "../src/helpers/af_array_proxy.hpp"
#include "../src/helpers/is_complex.hpp"

#include "../src/matched_filter/ipp_matched_filter.hpp"
#include "../src/matched_filter/af_matched_filter.hpp"

#include <type_traits>

namespace CorrelatorTests {
	template <typename T>
	class CorrelatorTest : public testing::Test {
	public:
		using Type = T;
	};
	using CorrelatorTypes = ::testing::Types<float, double>;
	TYPED_TEST_SUITE(CorrelatorTest, CorrelatorTypes);
	
	TYPED_TEST(CorrelatorTest, sequential_correlator) {
		const auto signal = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<std::complex<typename TestFixture::Type>>(0);
		const auto code = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<typename TestFixture::Type>(0);

		auto dst = ugsdr::SequentialCorrelator::Correlate(std::span(signal), std::span(code));

		ASSERT_FLOAT_EQ(dst.real(), code.size());
		ASSERT_FLOAT_EQ(dst.imag(), 0);
	}

	TYPED_TEST(CorrelatorTest, ipp_correlator) {
		const auto signal = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<std::complex<typename TestFixture::Type>>(0);
		const auto code = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<typename TestFixture::Type>(0);

		auto dst = ugsdr::IppCorrelator::Correlate(std::span(signal), std::span(code));

		ASSERT_FLOAT_EQ(dst.real(), code.size());
		ASSERT_FLOAT_EQ(dst.imag(), 0);
	}

	TYPED_TEST(CorrelatorTest, af_correlator) {
		const auto signal = ugsdr::Codegen<ugsdr::GpsL1Ca>::Get<std::complex<typename TestFixture::Type>>(0);
		const auto code = signal;

		auto dst = ugsdr::AfCorrelator::Correlate(ugsdr::ArrayProxy(signal), ugsdr::ArrayProxy(code));

		ASSERT_FLOAT_EQ(dst.real(), code.size());
		ASSERT_FLOAT_EQ(dst.imag(), 0);
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
		auto matched_filter = TestFixture::Type();

		const auto signal = ugsdr::Codegen<ugsdr::GlonassOf>::Get<std::complex<float>>(0);
		const auto code = ugsdr::Codegen<ugsdr::GlonassOf>::Get<float>(0);

		auto dst = matched_filter.Filter(signal, code);

		ASSERT_NEAR(dst[0].real(), code.size(), 1e-4);
		for (std::size_t i = 1; i < dst.size(); ++i)
			ASSERT_NEAR(dst[i].real(), -1.0, 1e-4);
	}
}
TEST(placeholder_tests, placeholder) {
	ASSERT_TRUE(true);
}
