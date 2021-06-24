#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/acquisition_parameters.hpp"
#include "prn_codes/GpsL1Ca.hpp"

#ifdef HAS_SIGNAL_PLOT
#include "signalsviewermdi.hpp"

namespace { CSignalsViewer* glob_sv = nullptr; }

template <typename ...Args>
void Add(Args&&... args) {
	glob_sv->Add(std::forward(args...));
}

void GenerateSignals(CSignalsViewer* sv) {
	glob_sv = sv;
#else
template <typename ...Args>
void Add(Args&&... args) {
}
	
	int main() {
#endif
	auto signal_parameters = ugsdr::SignalParametersBase<float>(R"(..\..\..\..\data\iq.bin)", ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2);
	auto data = signal_parameters.GetOneMs(0);

	sv->Add(L"Input signal", data);
	
	auto acquisition_parameters = ugsdr::AccquisitionParametersBase(signal_parameters, 5e3, 200);
	auto acquisition_results = acquisition_parameters.Process(0);

#ifndef HAS_SIGNAL_PLOT
	return 0;
#endif
}