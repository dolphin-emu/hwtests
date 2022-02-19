// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <network.h>
#include <stdarg.h>
#include <stdio.h>
#include <fmt/format.h>

#include "common/FormatUtil.h"

#define SERVER_PORT 16784

#define START_TEST() privStartTest(__FILE__, __LINE__)
#define DO_TEST(condition, fail_msg, ...)                                                          \
  privDoTest<Common::CountFmtReplacementFields(fail_msg)>(condition, __FILE__, __LINE__,           \
                                                          FMT_STRING(fail_msg), ##__VA_ARGS__)
#define END_TEST() privEndTest()

// private testing functions. Don't use these, but use the above macros, instead.
void privStartTest(const char* file, int line);
void privTestPassed();
void privTestFailed(const char* file, int line, const std::string& fail_msg);
template <std::size_t NumFields, typename... Args>
void privDoTest(bool condition, const char* file, int line, fmt::format_string<Args...> fail_msg, Args&&... args)
{
  static_assert(NumFields == sizeof...(args));
  if (condition)
    privTestPassed();
  else
    privTestFailed(file, line, fmt::format(fail_msg, std::forward<Args>(args)...));
}
void privEndTest();

void report_test_results();

void network_init();
void network_shutdown();
void network_vprintf(const char* str, va_list args);
void network_printf(const char* str, ...)
#ifndef _MSC_VER
    __attribute__((__format__(printf, 1, 2)))
#endif
    ;
