#include "gtest/gtest.h"

#include "../src/correlator/correlator.hpp"

#include "../src/matched_filter/matched_filter.hpp"
#include "../src/matched_filter/af_matched_filter.hpp"

#include "../src/mixer/table_mixer.hpp"

#include "../src/resample/ipp_resampler.hpp"

#include "../src/signal_parameters.hpp"

#include "../src/dfe/dfe.hpp"
#include "../src/acquisition/fse.hpp"

#include "../src/tracking/tracker.hpp"

#include "../src/measurements/measurement_engine.hpp"

#include "../src/positioning/standalone_rtklib.hpp"

#include <type_traits>

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

		struct SimplifiedAcquisitionResults {
			ugsdr::Sv sv;
			double doppler = 0.0;
			double code_offset = 0.0;

			template <typename T>
			bool Compare(const std::vector<ugsdr::AcquisitionResult<T>>& acquisition_result, double sampling_rate) const {
				auto it = std::find_if(acquisition_result.begin(), acquisition_result.end(), [this](auto& el) {
					return static_cast<std::uint32_t>(el.sv_number) == static_cast<std::uint32_t>(sv);	// TODO: implement proper operator==
				});
				if (it == acquisition_result.end())
					return false;
				auto doppler_offset = std::abs(doppler - it->doppler);
				if(doppler_offset > 400) {
					std::cout << "Doppler mismatch. Expected: " << it->doppler << ". Got : " << doppler << std::endl;
					return false;
				}
				auto code_diff = std::abs(code_offset - it->code_offset);
				if (code_diff > sampling_rate / ugsdr::TrackingParameters<ugsdr::DefaultTrackingParametersConfig, T>::GetCodePeriod(it->sv_number)) {
					std::cout << "Doppler mismatch. Expected: " << it->code_offset << ". Got : " << code_offset << std::endl;
					return false;
				}
				return true;
			}
		};

		auto GetResults(ugsdr::FileType file_type) {
			std::map<ugsdr::FileType, std::vector<SimplifiedAcquisitionResults>> map{};
			auto real_8 = std::vector<SimplifiedAcquisitionResults> {
				// From Development and Implementation of GPS Correlator Structures in MATLAB and Simulink with Focus on SDR Applications
				{ ugsdr::Sv(2, ugsdr::Signal::GpsCoarseAcquisition_L1),	0, 	34213 },
				{ ugsdr::Sv(5, ugsdr::Signal::GpsCoarseAcquisition_L1),	6000,	28203 },
				{ ugsdr::Sv(8, ugsdr::Signal::GpsCoarseAcquisition_L1),	-1000,	4695 },
				{ ugsdr::Sv(14, ugsdr::Signal::GpsCoarseAcquisition_L1),	0,	36321 },
				{ ugsdr::Sv(17, ugsdr::Signal::GpsCoarseAcquisition_L1),	2000,		20725 },
				{ ugsdr::Sv(20, ugsdr::Signal::GpsCoarseAcquisition_L1),	3000,	13404 },
				{ ugsdr::Sv(21, ugsdr::Signal::GpsCoarseAcquisition_L1),	0,	6289 },
				{ ugsdr::Sv(25, ugsdr::Signal::GpsCoarseAcquisition_L1),	5000,	26828 },
			};
			map[ugsdr::FileType::Real_8] = std::move(real_8);

			auto nt1065_first = std::vector<SimplifiedAcquisitionResults> {
				// From reference matlab receiver
				{ ugsdr::Sv(0, ugsdr::Signal::GpsCoarseAcquisition_L1),	1800, 	13803 },
				{ ugsdr::Sv(3, ugsdr::Signal::GpsCoarseAcquisition_L1),	-3100,	79481 },
				{ ugsdr::Sv(6, ugsdr::Signal::GpsCoarseAcquisition_L1),	-900,	38428 },
				{ ugsdr::Sv(8, ugsdr::Signal::GpsCoarseAcquisition_L1),	-3700,	38271 },
				{ ugsdr::Sv(10, ugsdr::Signal::GpsCoarseAcquisition_L1),	200,		22167 },
				{ ugsdr::Sv(16, ugsdr::Signal::GpsCoarseAcquisition_L1),	2500,	14095 },
				{ ugsdr::Sv(17, ugsdr::Signal::GpsCoarseAcquisition_L1),	400,	74879 },
				{ ugsdr::Sv(27, ugsdr::Signal::GpsCoarseAcquisition_L1),	400,	77135 },
				{ ugsdr::Sv(29, ugsdr::Signal::GpsCoarseAcquisition_L1),	900,	68055 },
			};
			map[ugsdr::FileType::Nt1065GrabberFirst] = std::move(nt1065_first);

			auto nt1065_third = std::vector<SimplifiedAcquisitionResults>();
			for(auto el : map[ugsdr::FileType::Nt1065GrabberFirst]) {
				el.doppler *= 1176.45 / 1575.42;
				nt1065_third.emplace_back(el);
			}

			map[ugsdr::FileType::Nt1065GrabberThird] = std::move(nt1065_third);

			return map.at(file_type);
		}


		template <typename T>
		bool VerifyResults(ugsdr::FileType file_type, const std::vector<ugsdr::AcquisitionResult<T>>& acquisition_result, double sampling_rate) {
			switch (file_type) {
			case ugsdr::FileType::Real_8:
			case ugsdr::FileType::Nt1065GrabberFirst:
			case ugsdr::FileType::Nt1065GrabberThird:
				break;

			case ugsdr::FileType::Iq_8_plus_8:
			case ugsdr::FileType::Iq_16_plus_16:
			case ugsdr::FileType::Nt1065GrabberSecond:
			case ugsdr::FileType::Nt1065GrabberFourth:
			case ugsdr::FileType::BbpDdc:
				return true;
			}

			const auto& results = GetResults(file_type);
			std::size_t cnt = 0;
			for (const auto& el : results)
				cnt += el.Compare(acquisition_result, sampling_rate);

			std::cout << "\t\tMatching acquisition result count:" << cnt << std::endl;
			return cnt >= 4;
		}

		enum class AcquistionTestType {
			MinimalFs,
			DefaultFs,
			FullFs
		};

		template <typename T, AcquistionTestType test_type = AcquistionTestType::DefaultFs>
		void TestAcquisition(ugsdr::FileType file_type, ugsdr::Signal signal, double doppler_range = 5e3) {
			auto signal_parameters = GetSignalParameters<T>(file_type);

			auto digital_frontend = ugsdr::DigitalFrontend(
				MakeChannel(signal_parameters, signal, signal_parameters.GetSamplingRate())
			);

			using FseConfig = std::conditional_t<test_type == AcquistionTestType::DefaultFs, ugsdr::DefaultFseConfig,
				std::conditional_t<test_type == AcquistionTestType::FullFs, ugsdr::ParametricFseConfig<1000000000>,
					ugsdr::ParametricFseConfig<2048000>>
			>;

			auto fse = ugsdr::FastSearchEngineBase<FseConfig, ugsdr::DefaultChannelConfig, T>(digital_frontend, doppler_range, 200);
			auto acquisition_results = fse.Process(false);

			ASSERT_FALSE(acquisition_results.empty());
			ASSERT_TRUE(VerifyResults(file_type, acquisition_results, signal_parameters.GetSamplingRate()));
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gps_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gps_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gln) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gln_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_8_plus_8_gln_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Iq_8_plus_8, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, iq_16_plus_16_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Iq_16_plus_16, ugsdr::Signal::GpsCoarseAcquisition_L1, 15e3);
		}

		TYPED_TEST(AcquisitionTest, iq_16_plus_16_gps_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Iq_16_plus_16, ugsdr::Signal::GpsCoarseAcquisition_L1, 15e3);
		}

		TYPED_TEST(AcquisitionTest, iq_16_plus_16_gps_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Iq_16_plus_16, ugsdr::Signal::GpsCoarseAcquisition_L1, 15e3);
		}

		TYPED_TEST(AcquisitionTest, real_8_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Real_8, ugsdr::Signal::GpsCoarseAcquisition_L1, 6e3);
		}

		TYPED_TEST(AcquisitionTest, real_8_gps_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Real_8, ugsdr::Signal::GpsCoarseAcquisition_L1, 6e3);
		}

		TYPED_TEST(AcquisitionTest, real_8_gps_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Real_8, ugsdr::Signal::GpsCoarseAcquisition_L1, 6e3);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gps) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberFirst, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gps_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Nt1065GrabberFirst, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gps_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Nt1065GrabberFirst, ugsdr::Signal::GpsCoarseAcquisition_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gps_L5) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberThird, ugsdr::Signal::Gps_L5I);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gln) {
			TestAcquisition<typename TestFixture::Type>(ugsdr::FileType::Nt1065GrabberSecond, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gln_full_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::FullFs>(ugsdr::FileType::Nt1065GrabberSecond, ugsdr::Signal::GlonassCivilFdma_L1);
		}

		TYPED_TEST(AcquisitionTest, nt1065_grabber_gln_min_fs) {
			TestAcquisition<typename TestFixture::Type, AcquistionTestType::MinimalFs>(ugsdr::FileType::Nt1065GrabberSecond, ugsdr::Signal::GlonassCivilFdma_L1);
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
			std::cout << "Delta: " << offset << " meters" << std::endl;
			ASSERT_LE(offset, 10);
		}



		TYPED_TEST(PositioningTest, TexCup) {
			auto signal_parameters = ugsdr::SignalParametersBase<typename TestFixture::Type>(SIGNAL_DATA_PATH + std::string("ntlab.bin"), ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6);
			auto signal_parameters_gln = ugsdr::SignalParametersBase<typename TestFixture::Type>(SIGNAL_DATA_PATH + std::string("ntlab.bin"), ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 79.5e6);

			auto digital_frontend = ugsdr::DigitalFrontend(
				MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::GpsCoarseAcquisition_L1 }, signal_parameters.GetSamplingRate() / 10),
				MakeChannel(signal_parameters_gln, std::vector{ ugsdr::Signal::GlonassCivilFdma_L1 }, signal_parameters_gln.GetSamplingRate() / 10)
			);

			auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
			auto acquisition_results = fse.Process();
			ASSERT_FALSE(acquisition_results.empty());
			auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
			tracker.Track(signal_parameters.GetNumberOfEpochs());

			auto measurement_engine = ugsdr::MeasurementEngine(tracker.GetTrackingParameters());
			auto positioning_engine = ugsdr::StandaloneRtklib(measurement_engine);
			auto reference_position = std::vector{ -741212.398, -5462378.228, 3197925.269 };	// ublox reference

			auto pos_and_time = positioning_engine.EstimatePosition(signal_parameters.GetNumberOfEpochs() - 1);
			auto pos = std::vector{ std::get<0>(pos_and_time), std::get<1>(pos_and_time), std::get<2>(pos_and_time) };

			for (std::size_t i = 0; i < pos.size(); ++i)
				pos[i] -= reference_position[i];
			auto offset = std::sqrt(std::accumulate(pos.begin(), pos.end(), 0.0, [](const auto& sum, const auto& val) {
				return sum + val * val;
			}));
			std::cout << "Delta: " << offset << " meters" << std::endl;
			ASSERT_LE(offset, 10);
		}
	}

}
