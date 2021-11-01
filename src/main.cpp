#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/fse.hpp"
#include "prn_codes/GpsL1Ca.hpp"
#include "serialization/serialization.hpp"
#include "tracking/tracker.hpp"
#include "dfe/dfe.hpp"
#include "measurements/measurement_engine.hpp"

#include <chrono>

#ifdef HAS_SIGNAL_PLOT
void GenerateSignals(CSignalsViewer* sv) {
	ugsdr::glob_sv = sv;
#else
int main() {
#endif
	auto signal_parameters =     ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6);
	auto signal_parameters_gln = ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 79.5e6);
	auto signal_parameters_L5 =  ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberThird, 1200e6, 79.5e6);
	auto signal_parameters_L2 =  ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberFourth, 1200e6, 79.5e6);

	auto digital_frontend = ugsdr::DigitalFrontend(
		MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::Galileo_E1b }, signal_parameters.GetSamplingRate() / 5),
		MakeChannel(signal_parameters_L5, std::vector{ ugsdr::Signal::Galileo_E5aI }, signal_parameters_L5.GetSamplingRate())
		//MakeChannel(signal_parameters_gln, ugsdr::Signal::GlonassCivilFdma_L1, signal_parameters_gln.GetSamplingRate() / 6),
		//MakeChannel(signal_parameters_L2, ugsdr::Signal::GlonassCivilFdma_L2, signal_parameters_L2.GetSamplingRate() / 6)
	);

#if 1
	auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
	auto acquisition_results = fse.Process(false);
	ugsdr::Save("acquisition_results_cache", acquisition_results);
#else
	std::vector<ugsdr::AcquisitionResult<float>> acquisition_results;
	ugsdr::Load("acquisition_results_cache", acquisition_results);
#endif

#if 1
	auto pre = std::chrono::system_clock::now();
	auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
	tracker.Track(1000 + 0 * signal_parameters.GetNumberOfEpochs());
	tracker.Plot();
	auto post = std::chrono::system_clock::now();
	ugsdr::Save("tracking_results_cache", tracker.GetTrackingParameters());

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() / static_cast<double>(signal_parameters.GetNumberOfEpochs()) * 100.0 << std::endl;
# else
	std::vector<ugsdr::TrackingParameters<float>> tracking_parameters;
	ugsdr::Load("tracking_results_cache", tracking_parameters);
	tracking_parameters.resize(1);
	for (auto& el : tracking_parameters)
		ugsdr::Add(L"Prompt tracking result", el.prompt);
#endif
	
	//auto measurement_engine = ugsdr::MeasurementEngine(tracker.GetTrackingParameters());

	//std::exit(0);

#ifndef HAS_SIGNAL_PLOT
	return 0;
#endif
}