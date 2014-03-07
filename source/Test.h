// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "stdio.h"
#include "stdarg.h"

#pragma once

#define START_TEST() privStartTest(__FILE__, __LINE__)
#define DO_TEST(condition, fail_msg, ...) privDoTest(condition, __FILE__, __LINE__, fail_msg, __VA_ARGS__)
#define END_TEST() privEndTest()
#define SIMPLE_TEST()

struct TestStatus
{
	TestStatus(const char* file, int line) : num_passes(0), num_failures(0), num_subtests(0), file(file), line(line)
	{
	}

	int num_passes;
	int num_failures;
	int num_subtests;

	const char* file;
	int line;
};

static TestStatus status(NULL, 0);
static int number_of_tests = 0;

void privStartTest(const char* file, int line)
{
	status = TestStatus(file, line);

	number_of_tests++;
}

void privDoTest(bool condition, const char* file, int line, const char* fail_msg, ...)
{
	va_list arglist;
	va_start(arglist, fail_msg);

	++status.num_subtests;

	if (condition)
	{
		++status.num_passes;
	}
	else
	{
		++status.num_failures;

		// TODO: vprintf forwarding doesn't seem to work?
		printf("Subtest %d failed in %s on line %d: ", status.num_subtests, file, line);
		vprintf(fail_msg, arglist);
		printf("\n");
	}
	va_end(arglist);
}

void privEndTest()
{
	if (0 == status.num_failures)
	{
		printf("Test %d passed (%d subtests)\n", number_of_tests, status.num_subtests);
	}
	else
	{
		printf("Test %d failed (%d subtests, %d failures)\n", number_of_tests, status.num_subtests, status.num_failures);
	}
}

void privSimpleTest(bool condition, const char* file, int line, const char* fail_msg, ...)
{
	// TODO
}
