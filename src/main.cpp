#include "common.hpp"
#include "signal_parameters.hpp"
#include "acquisition/fse.hpp"
#include "prn_codes/GpsL1Ca.hpp"
#include "tracking/tracker.hpp"
#include "dfe/dfe.hpp"

#ifdef HAS_SIGNAL_PLOT
void GenerateSignals(CSignalsViewer* sv) {
	ugsdr::glob_sv = sv;
#else
int main() {
#endif
	auto signal_parameters = ugsdr::SignalParametersBase<float>(R"(..\..\..\..\data\iq.bin)", ugsdr::FileType::Iq_8_plus_8, 1590e6, 79.5e6 / 2);

	auto digital_frontend = ugsdr::DigitalFrontend(
		MakeChannel(signal_parameters, ugsdr::Signal::GpsCoarseAcquisition_L1),
		MakeChannel(signal_parameters, ugsdr::Signal::GlonassCivilFdma_L1)
	);

#if 0
	auto fse = ugsdr::FastSearchEngineBase(digital_frontend, 5e3, 200);
	auto acquisition_results = fse.Process(false);
#else
	std::vector<ugsdr::AcquisitionResult<float>> acquisition_results(17);
	{
		acquisition_results[0].sv_number = ugsdr::Sv{ 0 ,ugsdr::System::Gps };
		acquisition_results[0].doppler = -14578200.000000000;
		acquisition_results[0].code_offset = 32840.000000000000;
		acquisition_results[0].level = 55227.324218750000;
		acquisition_results[0].sigma = 8255.9775390625000;
		acquisition_results[0].intermediate_frequency = -14580000.000000000;
		acquisition_results[1].sv_number = ugsdr::Sv{ 3 , ugsdr::System::Gps };
		acquisition_results[1].doppler = -14583200.000000000;
		acquisition_results[1].code_offset = 10.000000000000000;
		acquisition_results[1].level = 50017.578125000000;
		acquisition_results[1].sigma = 8459.2441406250000;
		acquisition_results[1].intermediate_frequency = -14580000.000000000;
		acquisition_results[2].sv_number = ugsdr::Sv{ 6 , ugsdr::System::Gps };
		acquisition_results[2].doppler = -14581000.000000000;
		acquisition_results[2].code_offset = 20530.000000000000;
		acquisition_results[2].level = 56138.500000000000;
		acquisition_results[2].sigma = 7760.8774414062500;
		acquisition_results[2].intermediate_frequency = -14580000.000000000;
		acquisition_results[3].sv_number = ugsdr::Sv{ 7 , ugsdr::System::Gps };
		acquisition_results[3].doppler = -14583400.000000000;
		acquisition_results[3].code_offset = 2830.0000000000000;
		acquisition_results[3].level = 33879.718750000000;
		acquisition_results[3].sigma = 7316.3881835937500;
		acquisition_results[3].intermediate_frequency = -14580000.000000000;
		acquisition_results[4].sv_number = ugsdr::Sv{ 8 , ugsdr::System::Gps };
		acquisition_results[4].doppler = -14583600.000000000;
		acquisition_results[4].code_offset = 20610.000000000000;
		acquisition_results[4].level = 29288.804687500000;
		acquisition_results[4].sigma = 7943.7294921875000;
		acquisition_results[4].intermediate_frequency = -14580000.000000000;
		acquisition_results[5].sv_number = ugsdr::Sv{ 10 , ugsdr::System::Gps };
		acquisition_results[5].doppler = -14579800.000000000;
		acquisition_results[5].code_offset = 28660.000000000000;
		acquisition_results[5].level = 87487.148437500000;
		acquisition_results[5].sigma = 9396.4472656250000;
		acquisition_results[5].intermediate_frequency = -14580000.000000000;
		acquisition_results[6].sv_number = ugsdr::Sv{ 12 , ugsdr::System::Gps };
		acquisition_results[6].doppler = -14577800.000000000;
		acquisition_results[6].code_offset = 17000.000000000000;
		acquisition_results[6].level = 32698.660156250000;
		acquisition_results[6].sigma = 9029.6796875000000;
		acquisition_results[6].intermediate_frequency = -14580000.000000000;
		acquisition_results[7].sv_number = ugsdr::Sv{ 16 , ugsdr::System::Gps };
		acquisition_results[7].doppler = -14577400.000000000;
		acquisition_results[7].code_offset = 32700.000000000000;
		acquisition_results[7].level = 60496.449218750000;
		acquisition_results[7].sigma = 7885.6103515625000;
		acquisition_results[7].intermediate_frequency = -14580000.000000000;
		acquisition_results[8].sv_number = ugsdr::Sv{ 17 , ugsdr::System::Gps };
		acquisition_results[8].doppler = -14579600.000000000;
		acquisition_results[8].code_offset = 2310.0000000000000;
		acquisition_results[8].level = 43543.093750000000;
		acquisition_results[8].sigma = 7785.2636718750000;
		acquisition_results[8].intermediate_frequency = -14580000.000000000;
		acquisition_results[9].sv_number = ugsdr::Sv{ 27 , ugsdr::System::Gps };
		acquisition_results[9].doppler = -14579600.000000000;
		acquisition_results[9].code_offset = 1180.0000000000000;
		acquisition_results[9].level = 69599.375000000000;
		acquisition_results[9].sigma = 8503.0556640625000;
		acquisition_results[9].intermediate_frequency = -14580000.000000000;
		acquisition_results[10].sv_number = ugsdr::Sv{ 29 , ugsdr::System::Gps };
		acquisition_results[10].doppler = -14579000.000000000;
		acquisition_results[10].code_offset = 10540.000000000000;
		acquisition_results[10].level = 57044.082031250000;
		acquisition_results[10].sigma = 7725.1215820312500;
		acquisition_results[10].intermediate_frequency = -14580000.000000000;
		acquisition_results[11].sv_number = ugsdr::Sv{ -3 , ugsdr::System::Glonass };
		acquisition_results[11].doppler = 10311300.000000000;
		acquisition_results[11].code_offset = 25310.000000000000;
		acquisition_results[11].level = 52812.101562500000;
		acquisition_results[11].sigma = 7244.0068359375000;
		acquisition_results[11].intermediate_frequency = 10312500.000000000;
		acquisition_results[12].sv_number = ugsdr::Sv{ -2 , ugsdr::System::Glonass };
		acquisition_results[12].doppler = 10877400.000000000;
		acquisition_results[12].code_offset = 31400.000000000000;
		acquisition_results[12].level = 37424.238281250000;
		acquisition_results[12].sigma = 6635.4609375000000;
		acquisition_results[12].intermediate_frequency = 10875000.000000000;
		acquisition_results[13].sv_number = ugsdr::Sv{ -1 , ugsdr::System::Glonass };
		acquisition_results[13].doppler = 11437700.000000000;
		acquisition_results[13].code_offset = 14470.000000000000;
		acquisition_results[13].level = 31658.154296875000;
		acquisition_results[13].sigma = 6529.5556640625000;
		acquisition_results[13].intermediate_frequency = 11437500.000000000;
		acquisition_results[14].sv_number = ugsdr::Sv{ 0 , ugsdr::System::Glonass };
		acquisition_results[14].doppler = 11998000.000000000;
		acquisition_results[14].code_offset = 2320.0000000000000;
		acquisition_results[14].level = 36495.308593750000;
		acquisition_results[14].sigma = 6714.8623046875000;
		acquisition_results[14].intermediate_frequency = 12000000.000000000;
		acquisition_results[15].sv_number = ugsdr::Sv{ 4 , ugsdr::System::Glonass };
		acquisition_results[15].doppler = 14245800.000000000;
		acquisition_results[15].code_offset = 33470.000000000000;
		acquisition_results[15].level = 35828.031250000000;
		acquisition_results[15].sigma = 6888.7402343750000;
		acquisition_results[15].intermediate_frequency = 14250000.000000000;
		acquisition_results[16].sv_number = ugsdr::Sv{ 5 , ugsdr::System::Glonass };
		acquisition_results[16].doppler = 14811500.000000000;
		acquisition_results[16].code_offset = 18470.000000000000;
		acquisition_results[16].level = 24222.138671875000;
		acquisition_results[16].sigma = 6697.3046875000000;
		acquisition_results[16].intermediate_frequency = 14812500.000000000;
	}
#endif

	auto pre = std::chrono::system_clock::now();
	auto tracker = ugsdr::Tracker(digital_frontend, acquisition_results);
	tracker.Track(signal_parameters.GetNumberOfEpochs());
	auto post = std::chrono::system_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(post - pre).count() / static_cast<double>(signal_parameters.GetNumberOfEpochs()) * 100.0 << std::endl;

	//std::exit(0);

#ifndef HAS_SIGNAL_PLOT
	return 0;
#endif
}