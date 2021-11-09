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
	//auto signal_parameters =     ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberFirst, 1590e6, 79.5e6);
	//auto signal_parameters_gln = ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberSecond, 1590e6, 79.5e6);
	//auto signal_parameters_L5 =  ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberThird, 1200e6, 79.5e6);
	//auto signal_parameters_L2 =  ugsdr::SignalParametersBase<float>(R"(../../../../data/nt1065_grabber.bin)", ugsdr::FileType::Nt1065GrabberFourth, 1200e6, 79.5e6);
	auto signal_parameters =     ugsdr::SignalParametersBase<float>(R"(../../../../data/bbp_ddc_gps_L1.dat)", ugsdr::FileType::BbpDdc, 1575.42e6, 33.25e6);
	auto signal_parameters_L2 =     ugsdr::SignalParametersBase<float>(R"(../../../../data/bbp_ddc_gps_L2.dat)", ugsdr::FileType::BbpDdc, 1227.6e6, 33.25e6);
	auto signal_parameters_L5 =     ugsdr::SignalParametersBase<float>(R"(../../../../data/bbp_ddc_gps_L5.dat)", ugsdr::FileType::BbpDdc, 1176.45e6, 66.5e6);

	auto digital_frontend = ugsdr::DigitalFrontend(
		MakeChannel(signal_parameters, std::vector{ ugsdr::Signal::QzssCoarseAcquisition_L1, ugsdr::Signal::Qzss_L1S }, signal_parameters.GetSamplingRate()),
		MakeChannel(signal_parameters_L2, std::vector{ ugsdr::Signal::Qzss_L2CM }, signal_parameters_L2.GetSamplingRate()),
		MakeChannel(signal_parameters_L5, std::vector{ ugsdr::Signal::Qzss_L5I, ugsdr::Signal::Qzss_L5Q }, signal_parameters_L5.GetSamplingRate())
	);

#if 1
	auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
	auto acquisition_results = fse.Process(true);
	if (acquisition_results.empty())
		return;
	ugsdr::Save("acquisition_results_cache", acquisition_results);
#else
	std::vector<ugsdr::AcquisitionResult<float>> acquisition_results;
	ugsdr::Load("acquisition_results_cache", acquisition_results);
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