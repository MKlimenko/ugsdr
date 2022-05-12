#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/fse.hpp"
#include "prn_codes/GpsL1Ca.hpp"
#include "serialization/serialization.hpp"
#include "tracking/tracker.hpp"
#include "dfe/dfe.hpp"
#include "measurements/measurement_engine.hpp"
#include "positioning/standalone_rtklib.hpp"

#include "math/stft.hpp"
#include "math/ipp_stft.hpp"

#include "antijamming/jamming_detection.hpp"

#include <chrono>
#include <cmath>

template <typename T>
static auto GenerateDetectionWindow(std::size_t window_size) {
	std::vector<T> detection_window(window_size);
	constexpr std::array<double, 4> a = { 0.3635819, 0.4891775, 0.1365995, 0.0106411 };

	for (std::size_t i = 0; i < window_size; ++i)
		for (std::size_t j = 0; j < a.size(); ++j)
			detection_window[i] += a[j] * std::pow(-1, j) * cos(2.0 * j * std::numbers::pi * i / (window_size - 1));

	return detection_window;
		}

#ifndef HAS_SIGNAL_PLOT
void main_impl() {
#else
void GenerateSignals(CSignalsViewer * sv) {
	ugsdr::glob_sv = sv;
#endif
	auto signal_parameters = ugsdr::SignalParametersBase<float>(SIGNAL_DATA_PATH + std::string("ntlab.bin"), ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6);
	//auto signal_parameters = ugsdr::SignalParametersBase<float>(SIGNAL_DATA_PATH + std::string("ra4_signal.bin"), ugsdr::FileType::Real_8, 1590e6, 53e6);
	//auto signal_parameters = ugsdr::SignalParametersBase<float>(R"(D:\Git\antijamming\models\data\msos804a\alligator_downsampled.bin)", 
	//	ugsdr::FileType::Iq_8_plus_8, 1575.42e6, 200e6);

	//auto narrowband_interference = std::make_unique<ugsdr::AdditionalTone<float>>(13.58e6, signal_parameters.GetSamplingRate(), 100);
	auto narrowband_interference = std::make_unique<ugsdr::AdditionalChirp<float>>(0, 20e6, signal_parameters.GetSamplingRate(), 20e-6, 100);

	auto digital_frontend = ugsdr::DigitalFrontend(
		MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::GpsCoarseAcquisition_L1 }, signal_parameters.GetSamplingRate(), narrowband_interference.get())
		//MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::GpsCoarseAcquisition_L1 }, signal_parameters.GetSamplingRate())
	);

	
	ugsdr::JammingDetector<std::complex<float>> detector(signal_parameters.GetSamplingRate());
	auto q = detector.Process(digital_frontend.GetEpoch(0).GetSubband(ugsdr::Signal::GpsCoarseAcquisition_L1));


	//auto signal = digital_frontend.GetEpoch(0).GetSubband(ugsdr::Signal::GpsCoarseAcquisition_L1);
	//ugsdr::Add(signal);
	//auto stft = ugsdr::IppStft::Transform(signal);
	//ugsdr::Add(stft[0]);
	//auto inv = ugsdr::IppStft::Transform(stft);
	//ugsdr::Add(inv);
	auto partial_data = digital_frontend.GetEpoch(0).GetSubband(ugsdr::Signal::GpsCoarseAcquisition_L1);
	ugsdr::Add(partial_data, signal_parameters.GetSamplingRate());
	auto stft = ugsdr::IppStft::Transform(partial_data);
	for (std::size_t i = 0; i < 3; ++i)
		ugsdr::Add(stft[i]);

	return;

#if 1
	auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
	auto acquisition_results = fse.Process(true);
	if (acquisition_results.empty())
		return;
	return;
	ugsdr::Save("acquisition_results_cache", acquisition_results);
#else
	std::vector<ugsdr::AcquisitionResult<float>> acquisition_results;
	ugsdr::Load("acquisition_results_cache_L5", acquisition_results);
#endif

#if 1
	auto pre = std::chrono::system_clock::now();
	auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
	tracker.Track(signal_parameters.GetNumberOfEpochs());
	tracker.Plot();
	auto post = std::chrono::system_clock::now();
	ugsdr::Save("tracking_results_cache", tracker.GetTrackingParameters());

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() / static_cast<double>(signal_parameters.GetNumberOfEpochs()) * 100.0 << std::endl;

	return;
	auto measurement_engine = ugsdr::MeasurementEngine(tracker.GetTrackingParameters());
	measurement_engine.WriteRinex(1000);
	auto positioning_engine = ugsdr::StandaloneRtklib(measurement_engine);

	positioning_engine.EstimatePosition(0);
#else
	std::vector<ugsdr::TrackingParameters<ugsdr::DefaultTrackingParametersConfig, float>> tracking_parameters;
	ugsdr::Load("tracking_results_cache_L5", tracking_parameters);
	//tracking_parameters.resize(1);
	//for (auto& el : tracking_parameters)
	//	ugsdr::Add(L"Prompt tracking result", el.prompt);

	auto measurement_engine = ugsdr::MeasurementEngine(tracking_parameters);
	measurement_engine.WriteRinex(1000);
	auto positioning_engine = ugsdr::StandaloneRtklib(measurement_engine);
	
	for(std::size_t i = 0; i < tracking_parameters[0].code_frequencies.size(); ++i)
		auto pos = positioning_engine.EstimatePosition(i);
#endif
	
	//std::exit(0);
}

#ifndef HAS_SIGNAL_PLOT
int main() {
	main_impl();
	return 0;
}
#endif

