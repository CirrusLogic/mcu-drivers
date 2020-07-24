/**
 * @file main.c
 *
 * @brief The main function for CS35L41 System Test Harness
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
#include "hw_0_bsp.h"
#include "otp_unpacker.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_CAL_PDN               (0)
#define APP_STATE_CAL_BOOTED            (1)
#define APP_STATE_CAL_PUP               (2)
#define APP_STATE_CALIBRATED            (3)
#define APP_STATE_PDN                   (4)
#define APP_STATE_BOOTED                (5)
#define APP_STATE_PUP                   (6)
#define APP_STATE_MUTE                  (7)
#define APP_STATE_UNMUTE                (8)
#define APP_STATE_HIBERNATE             (9)
#define APP_STATE_WAKE                  (10)
#define APP_STATE_CHECK_PROCESSING      (11)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t i2c_buffer[8];

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/*
 * The 2 functions l41_reset() and l41_read_otp are more fully implemented in cs35l41.c : cs35l41_reset().
 * Please see that source code for full implementation.
 */
static void l41_reset(void)
{
    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(BSP_GPIO_ID_DUT_RESET, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(CS35L41_T_RLPW_MS, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (1ms)
    bsp_driver_if_g->set_gpio(BSP_GPIO_ID_DUT_RESET, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(CS35L41_T_IRS_MS, NULL, NULL);
    bsp_driver_if_g->set_timer((CS35L41_POLL_OTP_BOOT_DONE_MS * 5), NULL, NULL);

    return;
}

/*
 * The 2 functions l41_reset() and l41_read_otp are more fully implemented in cs35l41.c : cs35l41_reset().
 * Please see that source code for full implementation.
 */
static uint32_t l41_read_otp(uint8_t *otp_buffer)
{
    uint32_t ret;

    i2c_buffer[0] = GET_BYTE_FROM_WORD(CS35L41_OTP_IF_OTP_MEM0_REG, 3);
    i2c_buffer[1] = GET_BYTE_FROM_WORD(CS35L41_OTP_IF_OTP_MEM0_REG, 2);
    i2c_buffer[2] = GET_BYTE_FROM_WORD(CS35L41_OTP_IF_OTP_MEM0_REG, 1);
    i2c_buffer[3] = GET_BYTE_FROM_WORD(CS35L41_OTP_IF_OTP_MEM0_REG, 0);

    ret = bsp_driver_if_g->i2c_read_repeated_start(BSP_DUT_DEV_ID,
                                                   i2c_buffer,
                                                   4,
                                                   otp_buffer,
                                                   (CS35L41_OTP_SIZE_WORDS * 4),
                                                   NULL,
                                                   NULL);

    return ret;
}

static uint32_t l41_read_reg(uint32_t addr, uint32_t *val)
{
    uint32_t ret = 1;

    i2c_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    i2c_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    i2c_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    i2c_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    ret = bsp_driver_if_g->i2c_read_repeated_start(BSP_DUT_DEV_ID,
                                                   i2c_buffer,
                                                   4,
                                                   (i2c_buffer + 4),
                                                   4,
                                                   NULL,
                                                   NULL);

    if (BSP_STATUS_OK == ret)
    {
        ADD_BYTE_TO_WORD(*val, i2c_buffer[4], 3);
        ADD_BYTE_TO_WORD(*val, i2c_buffer[5], 2);
        ADD_BYTE_TO_WORD(*val, i2c_buffer[6], 1);
        ADD_BYTE_TO_WORD(*val, i2c_buffer[7], 0);

        ret = 0;
    }

    return ret;
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
    uint8_t total_reg_entries = 0;
    uint8_t *otp_buffer;

    bsp_initialize(NULL, NULL);

    otp_buffer = malloc(sizeof(uint8_t) * OTP_UNPACKER_OTP_SIZE_WORDS * 4);

    l41_reset();
    l41_read_otp(otp_buffer);

    otp_unpacker_initialize(cs35l41_otp_maps[0].id, otp_buffer);
    otp_unpacker_get_reg_list_total(&total_reg_entries);
    for (uint8_t i = 0; i < total_reg_entries; i++)
    {
        uint32_t address, value;

        otp_unpacker_get_reg_address(&address, i);
        /*
         * The function below is normally not a public API call for the L41 driver API exposed in cs35l41.h
         */
        l41_read_reg(address, &value);
        otp_unpacker_set_reg_value(i, value);
    }

    uint32_t *reg_list_ptr = NULL;
    uint32_t reg_list_total_words = 0;

    otp_unpacker_get_unpacked_reg_list(&reg_list_ptr, &reg_list_total_words);

    if (otp_buffer)
    {
        free(otp_buffer);
        otp_buffer = NULL;
    }

    /*
     *  'reg_list_ptr' now points to an array of type 'uint32_t' of size 'reg_list_total_words'.
     *  The contents are address/value pairs to write to external serial memory.  Psuedo-code for sending
     *  address/value pairs to the part is given below:
     *
     *  for (int i = 0; i < reg_list_total_words; i += 2)
     *  {
     *      // register address = reg_list_ptr[i], register value = reg_list_ptr[i + 1]
     *      cs35l41_write_reg(reg_list_ptr[i], reg_list_ptr[i + 1]);
     *  }
     */

    while (1)
    {
        bsp_sleep();
    }

    otp_unpacker_deinitialize();

    exit(1);

    return -1;
}
