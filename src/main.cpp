#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/fse.hpp"
#include "prn_codes/GpsL1Ca.hpp"
#include "tracking/tracker.hpp"

#ifdef HAS_SIGNAL_PLOT
void GenerateSignals(CSignalsViewer* sv) {
	ugsdr::glob_sv = sv;
#else
int main() {
#endif
	auto signal_parameters = ugsdr::SignalParametersBase<float>(R"(..\..\..\..\data\iq.bin)", ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2);
	auto data = signal_parameters.GetOneMs(0);

	ugsdr::Add(L"Input signal", data);

	auto fse = ugsdr::FastSearchEngineBase(signal_parameters, 5e3, 200);
	auto acquisition_results = fse.Process();

	auto pre = std::chrono::system_clock::now();
	auto tracker = ugsdr::Tracker(signal_parameters, acquisition_results);
	tracker.Track(signal_parameters.GetNumberOfEpochs());
	auto post = std::chrono::system_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() / static_cast<double>(signal_parameters.GetNumberOfEpochs()) * 100.0 << std::endl;

	//std::exit(0);

#ifndef HAS_SIGNAL_PLOT
	return 0;
#endif
}