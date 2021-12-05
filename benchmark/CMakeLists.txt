cmake_minimum_required (VERSION 3.16)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp	
)
find_package(benchmark CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE 
												${Boost_INCLUDE_DIRS} 
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES} 
												benchmark::benchmark 
												OpenMP::OpenMP_CXX
												rtklib
)

if (${ArrayFire_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE ArrayFire::afcuda)
	target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS})
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

if(${IPP_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE IPP::ippcc IPP::ippch IPP::ippcore)
	target_include_directories(${PROJECT_NAME} PRIVATE ${IPP_INCLUDE_DIRS})											
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
