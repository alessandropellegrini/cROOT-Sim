/**
 * @file test/integration/integration_parallel_multi.c
 *
 * @brief Test: integration test of the parallel runtime with actual concurrency
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <test.h>

#include <integration/model/application.h>

static const char *test_arguments[] = {
	"--lp",
	"256",
	"--wt",
	"2",
	"--no-bind",
	NULL
};

const struct test_config test_config = {
	.test_arguments = test_arguments
};
