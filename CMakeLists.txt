cmake_minimum_required(VERSION 3.5)

project(hwtests CXX ASM)

message(STATUS "Using toolchain file " ${CMAKE_TOOLCHAIN_FILE})

include_directories(${LIBOGCDIR}/include)
link_directories(${DEVKITPRO}/libogc/lib/wii)

set(MACHDEP "-DGEKKO -mrvl -mcpu=750 -meabi -mhard-float")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-function -O2 ${CMAKE_CXX_FLAGS} ${MACHDEP}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-function -O2 ${CMAKE_CXX_FLAGS} ${MACHDEP} -fdiagnostics-color")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

add_custom_target(run)

function(add_hwtest)
    set(one_value_args MODULE TEST)
    set(multi_value_args FILES)
    cmake_parse_arguments(add_hwtest "" "${one_value_args}" "${multi_value_args}" ${ARGN} )

    set(executable_name ${add_hwtest_MODULE}_${add_hwtest_TEST})

    add_executable(${executable_name} ${add_hwtest_FILES})
    target_link_libraries(${executable_name} hwtests_common wiiuse bte fat ogc m)
    add_custom_target(run_${executable_name} sh ${CMAKE_SOURCE_DIR}/run.sh ${executable_name}${CMAKE_EXECUTABLE_SUFFIX})
    add_dependencies(run_${executable_name} ${executable_name})
    add_dependencies(run run_${executable_name})
endfunction()

include_directories(./)

add_subdirectory(common)
add_subdirectory(cputest)
add_subdirectory(gxtest)
add_subdirectory(iostest)
