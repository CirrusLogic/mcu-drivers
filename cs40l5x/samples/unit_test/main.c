/**
 * @file main.c
 *
 * @brief The main function for CS40L5X Unit Test Harness
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
#include "unity_fixture.h"

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
	RUN_TEST_GROUP(cs40l5x_initialize);
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
/**
 * @brief The Main Entry Point
 *
 * @return result of running all Unity test groups
 */
int main(int argc, const char *argv[])
{
	return UnityMain(argc, argv, RunAllTests);
}
