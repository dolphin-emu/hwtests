# Test suite for the GameCube/Wii hardware

This requires devkitPPC and libogc to build. Released under GPLv2.

## Usage:

To build all tests, call `sh build.sh` from the root directory. This requires CMake to be installed on your system and the DEVKITPRO and DEVKITPRO environment variables to be defined on your system. Additionally, if libogc is not installed to ${DEVKITPRO}/libogc, a CMake variable called LIBOGCDIR needs to be specified.

You can send an individual test elf over network to a Wii running the Homebrew Channel by calling `make && make run` from the subdirectory. This requires the `wiiload` executable to be located in your system binary paths and the `WIILOAD` environment variable to hold the IP address of your Wii, e.g. `export WIILOAD=tcp:192.168.0.124`.

Test results are transfered via network over port 16784 and can e.g. be retrieved using `netcat 192.168.0.124 16784`.
