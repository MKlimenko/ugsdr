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

target_link_libraries(${PROJECT_NAME} benchmark::benchmark)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
