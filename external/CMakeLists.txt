cmake_minimum_required(VERSION 3.15)

project(rtklib VERSION 2.4.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

if(UNIX)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

file(GLOB_RECURSE RTKLIB_RCV_SRC "RTKLIB/src/rcv/*.c")

add_library(${PROJECT_NAME} STATIC 
			RTKLIB/src/rtklib.h
			RTKLIB/src/ephemeris.c
			RTKLIB/src/ionex.c
			RTKLIB/src/lambda.c
			RTKLIB/src/pntpos.c
			RTKLIB/src/ppp.c
			RTKLIB/src/ppp_ar.c
			RTKLIB/src/preceph.c
			RTKLIB/src/rcvraw.c
			RTKLIB/src/rinex.c
			RTKLIB/src/rtcm.c
			RTKLIB/src/rtcm2.c
			RTKLIB/src/rtcm3.c
			RTKLIB/src/rtcm3e.c
			RTKLIB/src/rtkcmn.c
			RTKLIB/src/rtkpos.c
			RTKLIB/src/sbas.c
			${RTKLIB_RCV_SRC}
)

target_include_directories(${PROJECT_NAME} PUBLIC RTKLIB/src)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32)
	target_link_libraries(${PROJECT_NAME} winmm ws2_32)
endif()
