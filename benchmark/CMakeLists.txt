cmake_minimum_required (VERSION 3.16)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp
							../src/prn_codes/GalileoMemoryCodes.cpp	
)

target_include_directories(${PROJECT_NAME} PRIVATE 
												${Boost_INCLUDE_DIRS} 
												${FFTW3_INCLUDE_DIRS}
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES} 
												benchmark::benchmark 
												rtklib
)

if (${ArrayFire_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE ArrayFire::af)
	target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS})
endif()

if (${gcem_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE gcem)
endif()

if (${TBB_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)
	target_include_directories(${PROJECT_NAME} PRIVATE ${TBB_INCLUDE_DIRS})
endif()

if(${IPP_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE IPP::ippcc IPP::ippch IPP::ippcore)
	target_include_directories(${PROJECT_NAME} PRIVATE ${IPP_INCLUDE_DIRS})
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE FFTW3::fftw3 FFTW3::fftw3f)											
endif()

if (${OpenMP_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
