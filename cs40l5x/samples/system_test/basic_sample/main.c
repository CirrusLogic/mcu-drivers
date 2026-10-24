/**
 * @file main.c
 *
 * @brief The main function for CS40L5X System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "platform_bsp.h"
#ifdef USE_BULLSEYE
#include "libcov.h"
#endif

#include "unity_fixture.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
/**
 * @brief Run all tests in each test group
 *
 * @param [in] void
 *
 * @return void
 */
static void RunAllTests(void)
{
	RUN_TEST_GROUP(sys_cs40l5x_initialize);
	RUN_TEST_GROUP(sys_cs40l5x_boot);
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * @brief The Main Entry Point from __main
 *  By this time, the RAM RW-Data section has been initialized by the ARM-provided __main function.
 *
 * @return N/A (does not return)
 */
int main(void)
{
	int ret_val = 0;
	uint32_t bsp_status;
	char str[20] = {0};

	int argc = 2;
	const char *argv[] = {"blah", "-v"};

	bsp_status = bsp_initialize(NULL, NULL);
	if (bsp_status != BSP_STATUS_OK) {
		exit(1);
	}

	printf("SYSTEM_TEST - waiting\n");

	do {
		scanf("%1s", str);
	} while (strcmp(str, "G"));

	printf("SYSTEM_TEST - starting\n");
#ifdef USE_BULLSEYE
	printf("SYSTEM_TEST - _BullseyeCoverage defined\n");
#endif

	ret_val = UnityMain(argc, argv, RunAllTests);

	printf("SYSTEM_TEST - completed\n");

#ifdef USE_BULLSEYE
	cov_term_without_break();
#endif

	while (1)
		;

	exit(ret_val);

	return ret_val;
}
