cmake_minimum_required (VERSION 3.10)

project (ugsdr_root)

enable_testing()
add_subdirectory ("src")
add_subdirectory ("test")
add_subdirectory ("benchmark")
