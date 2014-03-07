// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "stdio.h"
#include "stdarg.h"

#pragma once

#define START_TEST()
#define END_TEST()
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

void DoTest(bool condition, const char* fail_msg, ...)
{
	++status.num_subtests;

	if (condition)
	{
		++status.num_passes;
	}
	else
	{
		++status.num_failures;

		va_list arglist;
		va_start(arglist, fail_msg);
		printf("Subtest %d failed: ", status.num_subtests);
		vprintf(fail_msg, arglist);
		printf("\n");
		va_end(arglist);
	}
}

void privEndTest()
{
	if (0 == status.num_failures)
	{
		printf("Test %d passed (%d subtests)\n", number_of_tests, status.num_subtests);
	}
	else
	{
		printf("Test %d passed (%d subtests, %d failures)\n", number_of_tests, status.num_subtests, status.num_failures);
	}
}

void privSimpleTest()
{

}
