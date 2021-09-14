cmake_minimum_required (VERSION 3.10)

include(FetchContent)
FetchContent_Declare(googlebenchmark
                     GIT_REPOSITORY https://github.com/google/benchmark
        )
FetchContent_MakeAvailable(googlebenchmark)

project(ugsdr_benchmark)
add_executable (${PROJECT_NAME} 
							benchmark.cpp	
)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


target_link_libraries(${PROJECT_NAME} benchmark::benchmark)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
