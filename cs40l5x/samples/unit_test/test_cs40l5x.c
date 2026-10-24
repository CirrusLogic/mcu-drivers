/**
 * @file test_cs40l5x.c
 *
 * @brief The CS40L5X Driver Unit Test module
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
#include "cs40l5x.h"
#include "mock_bsp.h"
#include "unity.h"
#include "unity_fixture.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define DEBUG_SEG_FAULT
/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
// Set up mocks

// Reset parameters tested

// Run code under test

// Asserts

/***********************************************************************************************************************
 * cs40l5x_initialize Unit Tests
 **********************************************************************************************************************/
TEST_GROUP(cs40l5x_initialize);

TEST_SETUP(cs40l5x_initialize)
{
	mock_bsp_initialize();
}

TEST_TEAR_DOWN(cs40l5x_initialize)
{
}

TEST(cs40l5x_initialize, initialize_null_pointer)
{
	uint32_t status;

	// Run code under test
	status = cs40l5x_initialize(NULL);

	// Asserts
	TEST_ASSERT_EQUAL(CS40L5X_STATUS_FAIL, status);
}

TEST_GROUP_RUNNER(cs40l5x_initialize)
{
	RUN_TEST_CASE(cs40l5x_initialize, initialize_null_pointer);
}
