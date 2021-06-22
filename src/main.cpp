#include "common.hpp"
#include "signal_parameters.hpp"

int main() {
	auto signal_parameters = ugsdr::SignalParameters(R"(D:\Git\ugsdr\data\iq.bin)", ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2);
	auto data = signal_parameters.GetOneMs(0);

	auto signal_parameters_fp32 = ugsdr::SignalParametersBase<float>(R"(D:\Git\ugsdr\data\iq.bin)", ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2);
	auto data_fp32 = signal_parameters_fp32.GetOneMs(0);

	
	return 0;
}