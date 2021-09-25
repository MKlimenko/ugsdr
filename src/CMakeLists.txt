cmake_minimum_required (VERSION 3.10)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_result.hpp 
							acquisition/fse.hpp 
							helpers/af_array_proxy.hpp
							helpers/ipp_complex_type_converter.hpp
							helpers/is_complex.hpp
							helpers/visualizer.hpp 
							matched_filter/matched_filter.hpp 
							matched_filter/af_matched_filter.hpp
							matched_filter/ipp_matched_filter.hpp
							math/abs.hpp
							math/ipp_abs.hpp
							math/conj.hpp
							math/ipp_conj.hpp
							math/dft.hpp
							math/af_dft.hpp
							math/ipp_dft.hpp
							math/max_index.hpp
							math/ipp_max_index.hpp
							math/reshape_and_sum.hpp
							math/ipp_reshape_and_sum.hpp
							math/mean_stddev.hpp
							math/ipp_mean_stddev.hpp
							mixer/batch_mixer.hpp
							mixer/ipp_mixer.hpp
							mixer/mixer.hpp 
							prn_codes/GlonassOf.hpp 
							prn_codes/GpsL1Ca.hpp  
							prn_codes/codegen.hpp 
							prn_codes/lfsr.hpp 
							prn_codes/codegen_wrapper.hpp
							resample/decimator.hpp 
							resample/ipp_decimator.hpp 
							resample/resampler.hpp 
							resample/ipp_resampler.hpp 
							resample/upsampler.hpp 
							resample/ipp_upsampler.hpp 
							tracking/tracker.hpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

find_package(OpenMP REQUIRED)
find_package(ArrayFire REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX ArrayFire::afcuda)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PUBLIC -ftime-trace)
endif()

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/lib/signalplot64.lib)
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{SIGNALPLOT64}/include)
	add_definitions(-DHAS_SIGNAL_PLOT)
endif()