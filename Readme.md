# Test suite for the GameCube/Wii hardware

This requires devkitPPC and libogc to build. Released under GPLv2.

## Usage:

To build all tests, call `make` from the root directory.

You can send an individual test elf over network to a Wii running the Homebrew Channel by calling `make && make run` from the subdirectory. This requires the `wiiload` executable to be located in your system binary paths and the `WIILOAD` environment variable to hold the IP address of your Wii, e.g. `export WIILOAD=tcp:192.168.0.124`.
