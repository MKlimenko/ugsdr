cmake_minimum_required (VERSION 3.16)

project(ugsdr_test)
add_executable (${PROJECT_NAME} 
							basic_tests.cpp	
							integration_tests.cpp
							../src/prn_codes/GalileoMemoryCodes.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PUBLIC -ftime-trace)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
												${Boost_INCLUDE_DIRS}
												${FFTW3_INCLUDE_DIRS}
												../external/RTKLIB/src
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
												${Boost_LIBRARIES}  
												GTest::gtest 
												GTest::gtest_main 
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
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE FFTW3::fftw3 FFTW3::fftw3f)	
endif()

if (${OpenMP_FOUND})
	target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
file(DOWNLOAD 
#		http://agamenon.tsc.uah.es/Asignaturas/it/rd/apuntes/GPSdata-DiscreteComponents-fs38_192-if9_55.bin	# original link doesn't seem to work after 22Q2
		https://onedrive.live.com/download?cid=11492625001FE073&resid=11492625001FE073%21425&authkey=AFqDC_U7wJS5aUg
		${CMAKE_CURRENT_LIST_DIR}/../data/GPSdata-DiscreteComponents-fs38_192-if9_55.bin 
		SHOW_PROGRESS
		EXPECTED_MD5 f5b0f32d0f97b23060d62ea3b5dc86b2
)
file(DOWNLOAD 
#		https://rnl-data.ae.utexas.edu/texcup/2019May12-rover/ntlab/ntlab.bin # full link, switch after CMake 3.24.0 is out
		https://dl.dropboxusercontent.com/s/hfdza7a2rsbh77k/ntlab.bin
		${CMAKE_CURRENT_LIST_DIR}/../data/ntlab.bin
#		RANGE_END 4770000000 #CMake MR !6986, merged into master, should be available in 3.24.0
		SHOW_PROGRESS
		EXPECTED_MD5 8023faab3bba118d822af104dd02510c
)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

if (MSVC)
  add_definitions(/bigobj)
endif ()
