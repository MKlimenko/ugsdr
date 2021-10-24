cmake_minimum_required (VERSION 3.19)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp	
)
find_package(benchmark REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${IPP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} benchmark::benchmark OpenMP::OpenMP_CXX ArrayFire::afcuda gcem ${Boost_LIBRARIES} IPP::ippcore IPP::ippcc)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY ISPC_INSTRUCTION_SETS avx2)
