cmake_minimum_required(VERSION 2.6)

project(gxtest)

message(STATUS "Using toolchain file " ${CMAKE_TOOLCHAIN_FILE})

INCLUDE(CMakeForceCompiler) # TODO: Not sure if this is necessary

add_definitions(-std=c++11)

include_directories(${LIBOGCDIR}/include)
link_directories(${DEVKITPRO}/libogc/lib/wii)

set(MACHDEP "-DGEKKO -mrvl -mcpu=750 -meabi -mhard-float")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")

add_subdirectory(cputest)
add_subdirectory(gxtest)
