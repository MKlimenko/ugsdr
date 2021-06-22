cmake_minimum_required (VERSION 3.10)

project(ugsdr)

add_executable (${PROJECT_NAME} 
							main.cpp
							common.hpp
							signal_parameters.hpp
							acquisition/acquisition_parameters.hpp 
							acquisition/acquisition_result.hpp 
							mixer/mixer.hpp "prn_codes/lfsr.hpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
