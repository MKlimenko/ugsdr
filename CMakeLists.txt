cmake_minimum_required (VERSION 3.19)

project (ugsdr_root)

set(IPP_PATH "$ENV{ONEAPI_ROOT}ipp/latest/lib/cmake/ipp")
string(REPLACE "\\" "/" IPP_PATH ${IPP_PATH})
set(IPP_INCLUDE_DIRS "$ENV{ONEAPI_ROOT}ipp/latest/include")
string(REPLACE "\\" "/" IPP_INCLUDE_DIRS ${IPP_INCLUDE_DIRS})
set(IPP_SHARED FALSE)

find_package(ArrayFire REQUIRED)
find_package(Boost REQUIRED COMPONENTS system iostreams)
find_package(cereal REQUIRED)
find_package(gcem REQUIRED)
find_package(IPP REQUIRED PATHS ${IPP_PATH})
find_package(OpenMP REQUIRED)

if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

enable_testing()
add_subdirectory ("src")
add_subdirectory ("test")
add_subdirectory ("benchmark")
