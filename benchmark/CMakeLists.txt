cmake_minimum_required (VERSION 3.16)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp	
)
find_package(benchmark CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE 
												${ArrayFire_INCLUDE_DIRS} 
												${Boost_INCLUDE_DIRS} 
												${IPP_INCLUDE_DIRS}
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES} 
												benchmark::benchmark 
												IPP::ippcc 
												IPP::ippch
												IPP::ippcore
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

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
