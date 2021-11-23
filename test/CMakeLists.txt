cmake_minimum_required (VERSION 3.16)

project(ugsdr_test)
add_executable (${PROJECT_NAME} 
							test.cpp	
							../src/prn_codes/GalileoMemoryCodes.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PUBLIC -ftime-trace)
endif()

find_package(GTest CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE 
												${ArrayFire_INCLUDE_DIRS} 
												${Boost_INCLUDE_DIRS} 
												${IPP_INCLUDE_DIRS}
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES} 
												ArrayFire::afcuda
												gcem 
												GTest::gtest 
												GTest::gtest_main 
												IPP::ippcc
												IPP::ippch
												IPP::ippcore 
												OpenMP::OpenMP_CXX
												rtklib
												TBB::tbb
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})