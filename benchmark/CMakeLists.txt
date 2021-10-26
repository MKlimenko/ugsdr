cmake_minimum_required (VERSION 3.16)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp	
)
find_package(benchmark CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${IPP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark OpenMP::OpenMP_CXX ArrayFire::afcuda gcem ${Boost_LIBRARIES} IPP::ippcore IPP::ippcc)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
