cmake_minimum_required (VERSION 3.10)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_result.hpp 
							acquisition/fse.hpp 
							mixer/mixer.hpp 
							mixer/batch_mixer.hpp
							mixer/ipp_mixer.hpp
							prn_codes/lfsr.hpp 
							prn_codes/codegen.hpp 
							prn_codes/GpsL1Ca.hpp  
							prn_codes/GlonassOf.hpp 
							math/fft.hpp
							helpers/visualizer.hpp 
							matched_filter/matched_filter.hpp 
							math/upsample.hpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

IF (WIN32)
	target_link_libraries(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/lib/signalplot64.lib)
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/include)
	add_definitions(-DHAS_SIGNAL_PLOT)
ENDIF()