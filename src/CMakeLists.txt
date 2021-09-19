cmake_minimum_required (VERSION 3.10)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_result.hpp 
							acquisition/fse.hpp 
							helpers/visualizer.hpp 
							matched_filter/ipp_matched_filter.hpp
							matched_filter/matched_filter.hpp 
							math/ipp_complex_type_converter.hpp
							math/conj.hpp
							math/ipp_conj.hpp
							math/dft.hpp
							math/ipp_dft.hpp
							math/upsample.hpp 
							mixer/batch_mixer.hpp
							mixer/ipp_mixer.hpp
							mixer/mixer.hpp 
							prn_codes/GlonassOf.hpp 
							prn_codes/GpsL1Ca.hpp  
							prn_codes/codegen.hpp 
							prn_codes/lfsr.hpp 
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${KFR_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

IF (WIN32)
	target_link_libraries(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/lib/signalplot64.lib)
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/include)
	add_definitions(-DHAS_SIGNAL_PLOT)
ENDIF()