cmake_minimum_required (VERSION 3.16)

project (ugsdr_root)

set(IPP_PATH "$ENV{ONEAPI_ROOT}ipp/latest/lib/cmake/ipp")
string(REPLACE "\\" "/" IPP_PATH ${IPP_PATH})
set(IPP_INCLUDE_DIRS "$ENV{ONEAPI_ROOT}ipp/latest/include")
string(REPLACE "\\" "/" IPP_INCLUDE_DIRS ${IPP_INCLUDE_DIRS})
set(IPP_SHARED FALSE)

find_package(ArrayFire CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system iostreams)
find_package(cereal CONFIG REQUIRED)
find_package(gcem CONFIG REQUIRED)
find_package(IPP CONFIG REQUIRED PATHS ${IPP_PATH})
find_package(OpenMP REQUIRED)
find_package(TBB CONFIG REQUIRED)

if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_compile_definitions(ENAGLO ENAGAL ENAQZS ENACMP ENAIRN NFREQ=3 TRACE NOMINMAX _WINSOCK2API_)

enable_testing()
add_subdirectory ("src")
add_subdirectory ("test")
add_subdirectory ("benchmark")
add_subdirectory("external")
add_dependencies(ugsdr rtklib)
add_dependencies(ugsdr_test rtklib)
add_dependencies(ugsdr_benchmark rtklib)