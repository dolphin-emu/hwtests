// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <network.h>
#include <stdarg.h>
#include <stdio.h>

#pragma once

#define SERVER_PORT 16784

#define START_TEST() privStartTest(__FILE__, __LINE__)
#define DO_TEST(condition, fail_msg, ...)                                                          \
  privDoTest(condition, __FILE__, __LINE__, fail_msg, ##__VA_ARGS__)
#define END_TEST() privEndTest()
#define SIMPLE_TEST()

// private testing functions. Don't use these, but use the above macros, instead.
void privStartTest(const char* file, int line);
void privDoTest(bool condition, const char* file, int line, const char* fail_msg, ...)
#ifndef _MSC_VER
    __attribute__((__format__(printf, 4, 5)))
#endif
    ;
void privEndTest();
// TODO: Not implemented, yet
// void privSimpleTest(bool condition, const char* file, int line, const char* fail_msg, ...);

void network_init();
void network_shutdown();
void network_vprintf(const char* str, va_list args);
void network_printf(const char* str, ...)
#ifndef _MSC_VER
    __attribute__((__format__(printf, 1, 2)))
#endif
    ;
