cmake_minimum_required(VERSION 2.6)

project(hwtests)

message(STATUS "Using toolchain file " ${CMAKE_TOOLCHAIN_FILE})

add_definitions(-std=c++11)

include_directories(${LIBOGCDIR}/include)
link_directories(${DEVKITPRO}/libogc/lib/wii)

set(MACHDEP "-DGEKKO -mrvl -mcpu=750 -meabi -mhard-float")

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACHDEP}")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_custom_target(run)

function(add_hwtest module testname filelist)
    add_executable(${module}_${testname} ${filelist} ${COMMON_CODE})
    target_link_libraries(${module}_${testname} hwtests_common wiiuse bte fat ogc m)
    add_custom_target(run_${module}_${testname} sh ${CMAKE_SOURCE_DIR}/run.sh ${module}_${testname}${CMAKE_EXECUTABLE_SUFFIX})
    add_dependencies(run_${module}_${testname} ${module}_${testname})
    add_dependencies(run run_${module}_${testname})
endfunction()

include_directories(common)

add_subdirectory(common)
add_subdirectory(cputest)
add_subdirectory(gxtest)
