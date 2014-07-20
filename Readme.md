# Test suite for the GameCube/Wii hardware

Released under GPLv2.

## Requirements:

- CMake
- devkitPPC
- libogc (if not installed into ${DEVKITPRO}/libogc, a CMake variable called LIBOGCDIR needs to be specified)
- DEVKITPRO and DEVKITPRO environment variables (see devkitPPC documentation)

To use the run_* CMake targets:
- netcat
- WIILOAD environment variable with the IP address of your Wii, e.g., `export WIILOAD=tcp:192.168.0.124`

## Usage:

To build all tests, call `./build.sh` from the root directory.

Tests are run by sending their ELFs one-by-one over the network to a Wii running the Homebrew Channel. To do this, call `make run_$NAMEOFTEST` from the build directory.

To run all tests, call `make -j1 run` from the build directory. Note that there are some very slow tests.
