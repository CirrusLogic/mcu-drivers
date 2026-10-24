/**
 * @file test_cs40l5x.c
 *
 * @brief The CS40L5X Driver System Test module
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
#include "platform_bsp.h"
#include "unity.h"
#include "unity_fixture.h"

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

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
// Set up mocks

// Reset parameters tested

// Run code under test

// Asserts

/***********************************************************************************************************************
 * sys_cs40l5x_initialize System Tests
 **********************************************************************************************************************/
TEST_GROUP(sys_cs40l5x_initialize);

TEST_SETUP(sys_cs40l5x_initialize)
{
}

TEST_TEAR_DOWN(sys_cs40l5x_initialize)
{
}

TEST(sys_cs40l5x_initialize, ok)
{
	uint32_t status;

	// Run code under test
	status = bsp_dut_initialize();

	// Asserts
	TEST_ASSERT_EQUAL(BSP_STATUS_OK, status);
}

TEST_GROUP_RUNNER(sys_cs40l5x_initialize)
{
	RUN_TEST_CASE(sys_cs40l5x_initialize, ok);
}

/***********************************************************************************************************************
 * sys_cs40l5x_boot System Tests
 **********************************************************************************************************************/
TEST_GROUP(sys_cs40l5x_boot);

TEST_SETUP(sys_cs40l5x_boot)
{
	bsp_dut_initialize();
}

TEST_TEAR_DOWN(sys_cs40l5x_boot)
{
}

TEST(sys_cs40l5x_boot, boot_fw_and_coeff)
{
	uint32_t status;

	// Run code under test
	status = bsp_dut_reset();
	// bsp_dut_power_down();
	// status = bsp_dut_boot(false);

	// Asserts
	TEST_ASSERT_EQUAL(BSP_STATUS_OK, status);
}

TEST_GROUP_RUNNER(sys_cs40l5x_boot)
{
	RUN_TEST_CASE(sys_cs40l5x_boot, boot_fw_and_coeff);
}
