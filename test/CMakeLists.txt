cmake_minimum_required (VERSION 3.19)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)


project(ugsdr_test)
add_executable (${PROJECT_NAME} 
							test.cpp	
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${PROJECT_NAME} PUBLIC -ftime-trace)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${ArrayFire_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${IPP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX ArrayFire::afcuda gcem ${Boost_LIBRARIES} gtest_main IPP::ippcore IPP::ippcc IPP::ippch)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY ISPC_INSTRUCTION_SETS avx2)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})