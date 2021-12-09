cmake_minimum_required (VERSION 3.16)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_result.hpp 
							acquisition/fse.hpp 
							correlator/af_correlator.hpp
							correlator/correlator.hpp
							correlator/ipp_correlator.hpp
							dfe/dfe.hpp
							ephemeris/Ephemeris.hpp
							ephemeris/GlonassEphemeris.hpp
							ephemeris/GpsEphemeris.hpp
							helpers/af_array_proxy.hpp
							helpers/BbpPackedSpan.hpp
							helpers/ipp_complex_type_converter.hpp
							helpers/is_complex.hpp
							helpers/NtlabPackedSpan.hpp
							helpers/rtklib_helpers.hpp
							helpers/visualizer.hpp 
							matched_filter/af_matched_filter.hpp
							matched_filter/ipp_matched_filter.hpp
							matched_filter/matched_filter.hpp 
							math/abs.hpp
							math/af_dft.hpp
							math/conj.hpp
							math/dft.hpp
							math/ipp_abs.hpp
							math/ipp_conj.hpp
							math/ipp_dft.hpp
							math/ipp_max_index.hpp
							math/ipp_mean_stddev.hpp
							math/ipp_reshape_and_sum.hpp
							math/max_index.hpp
							math/mean_stddev.hpp
							math/reshape_and_sum.hpp
							measurements/measurement_engine.hpp
							measurements/observable.hpp
							measurements/timescale.hpp
							mixer/af_mixer.hpp
							mixer/batch_mixer.hpp
							mixer/ipp_mixer.hpp
							mixer/mixer.hpp 
							mixer/nco.hpp
							mixer/table_mixer.hpp
							positioning/standalone_engine.hpp
							positioning/standalone_rtklib.hpp
							prn_codes/BeiDouB1C.hpp
							prn_codes/BeiDouB1I.hpp
							prn_codes/codegen.hpp 
							prn_codes/codegen_wrapper.hpp
							prn_codes/GalileoE1b.hpp  
							prn_codes/GalileoE1c.hpp  
							prn_codes/GalileoE5.hpp 
							prn_codes/GalileoE5aI.hpp
							prn_codes/GalileoE5aQ.hpp
							prn_codes/GalileoE5bI.hpp
							prn_codes/GalileoE5bQ.hpp
							prn_codes/GalileoE6b.hpp  
							prn_codes/GalileoE6c.hpp  
							prn_codes/GalileoMemoryCodes.cpp
							prn_codes/GlonassOf.hpp 
							prn_codes/GoldCodes.hpp  
							prn_codes/GpsL1Ca.hpp 
							prn_codes/GpsL2CM.hpp 
							prn_codes/GpsL5I.hpp
							prn_codes/GpsL5Q.hpp
							prn_codes/L2CM.hpp  
							prn_codes/L5.hpp
							prn_codes/lfsr.hpp 
							prn_codes/MemoryCodes.hpp  
							prn_codes/NavICL5Ca.hpp
							prn_codes/QzssL1Ca.hpp
							prn_codes/QzssL1Saif.hpp
							prn_codes/QzssL2CM.hpp 
							prn_codes/QzssL5I.hpp
							prn_codes/QzssL5Q.hpp
							prn_codes/SbasL1Ca.hpp
							prn_codes/SbasL5I.hpp
							prn_codes/SbasL5Q.hpp
							prn_codes/Weil.hpp
							resample/decimator.hpp 
							resample/ipp_decimator.hpp 
							resample/ipp_resampler.hpp 
							resample/ipp_upsampler.hpp 
							resample/resampler.hpp 
							resample/upsampler.hpp 
							serialization/serialization.hpp
							tracking/tracker.hpp
							tracking/tracking_parameters.hpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PRIVATE 
												${ArrayFire_INCLUDE_DIRS} 
												${Boost_INCLUDE_DIRS} 
												${IPP_INCLUDE_DIRS}
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES}
												OpenMP::OpenMP_CXX
												rtklib
)

if (${ArrayFire_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE ArrayFire::afcuda)
endif()

if (${cereal_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE cereal)
endif()

if (${gcem_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE gcem)
endif()

if (${TBB_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)
endif()

if (${IPP_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE IPP::ippcc IPP::ippch IPP::ippcore)												
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PUBLIC -ftime-trace)
endif()

if (DEFINED ENV{SIGNALPLOT64})
	target_link_libraries(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/lib/signalplot64.lib)
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/include)
	add_definitions(-DHAS_SIGNAL_PLOT)
endif()