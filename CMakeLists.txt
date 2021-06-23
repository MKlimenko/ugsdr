cmake_minimum_required (VERSION 3.10)

project (ugsdr_root)
include(FindIPP.cmake)

include_directories(${IPP_INCLUDE_DIR})
link_libraries(${IPP_LIBRARIES})

enable_testing()
add_subdirectory ("src")
add_subdirectory ("test")
add_subdirectory ("benchmark")
