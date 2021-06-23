cmake_minimum_required (VERSION 3.10)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_parameters.hpp 
							acquisition/acquisition_result.hpp 
							mixer/mixer.hpp 
							mixer/batch_mixer.hpp
							mixer/ipp_mixer.hpp
							prn_codes/lfsr.hpp 
							prn_codes/GpsL1Ca.hpp  
							prn_codes/GlonassOf.hpp "prn_codes/codegen.hpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)