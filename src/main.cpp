#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/fse.hpp"
#include "prn_codes/GpsL1Ca.hpp"
#include "serialization/serialization.hpp"
#include "tracking/tracker.hpp"
#include "dfe/dfe.hpp"
#include "measurements/measurement_engine.hpp"
#include "positioning/standalone_rtklib.hpp"

#include <chrono>

#ifndef HAS_SIGNAL_PLOT
void main_impl() {
#else
void GenerateSignals(CSignalsViewer * sv) {
	ugsdr::glob_sv = sv;
#endif
	auto signal_parameters =  ugsdr::SignalParametersBase<float>(SIGNAL_DATA_PATH + std::string("ntlab.bin"), ugsdr::FileType::Nt1065GrabberThird, 1200e6, 79.5e6);

	auto digital_frontend = ugsdr::DigitalFrontend(
		MakeChannel(signal_parameters, std::vector{ugsdr::Signal::Gps_L5I}, signal_parameters.GetSamplingRate())
	);

#if 0
	auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
	auto acquisition_results = fse.Process(!true);
	if (acquisition_results.empty())
		return;
	ugsdr::Save("acquisition_results_cache_L5", acquisition_results);
#else
	std::vector<ugsdr::AcquisitionResult<float>> acquisition_results;
	ugsdr::Load("acquisition_results_cache_L5", acquisition_results);
#endif

#if 0
	auto pre = std::chrono::system_clock::now();
	auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
	tracker.Track(signal_parameters.GetNumberOfEpochs());
	tracker.Plot();
	auto post = std::chrono::system_clock::now();
	ugsdr::Save("tracking_results_cache_L5", tracker.GetTrackingParameters());

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

