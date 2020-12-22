/**
 * @file cs47l15.c
 *
 * @brief The CS47L15 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include "cs47l15.h"
#include "bsp_driver_if.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/**
 * @defgroup CS47L15_POLL_
 * @brief Polling constants for polling times and counts
 *
 * @{
 */
#define CS47L15_POLL_ACK_CTRL_MS                (10)    ///< Delay in ms between polling ACK controls
#define CS47L15_POLL_ACK_CTRL_MAX               (10)    ///< Maximum number of times to poll ACK controls
#define CS47L15_POLL_MEM_ENA_MS                 (250)   ///< Delay in ms between polling ACK controls
#define CS47L15_POLL_MEM_ENA_MAX                (10)    ///< Maximum number of times to poll ACK controls
/** @} */

/**
 * @defgroup CS47L15_REGION_LOCK_
 * @brief Region lock codes
 *
 * @{
 */
#define CS47L15_REGION_LOCK_CODE0                (0x5555)       ///< First code required to lock a region
#define CS47L15_REGION_LOCK_CODE1                (0xAAAA)       ///< Second code required to lock a region
#define CS47L15_REGION_LOCK_UPPER_SHIFT          (16)           ///< Shift required to update the second region in a region lock reg
/** @} */

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * CS47L15 RevA Register Patch Errata
 *
 * The array is in the form:
 * - word0 - 1st register address to patch
 * - word1 - 1st register value
 * - word2 - 2nd register address to patch
 * - word3 - 2nd register value
 * - ...
 *
 */
static const uint32_t cs47l15_reva_errata_patch[] =
{
    0x008C, 0x5555,
    0x008C, 0xAAAA,
    0x0314, 0x0080,
    0x04A8, 0x6023,
    0x04A9, 0x6023,
    0x04D4, 0x0008,
    0x04CF, 0x0F00,
    0x04D7, 0x1B2B,
    0x008C, 0xCCCC,
    0x008C, 0x3333,
};

/**
* CS47L15 interrupt regs to check
*
* Each element is in format of {irq register offset from base, mask, flag associated with this event}
*
* @see cs47l15_event_handler
*/
const irq_reg_t cs47l15_event_data[] =
{
    {0x00, CS47L15_BOOT_DONE_STS1_MASK         , CS47L15_EVENT_FLAG_BOOT_DONE},        //< CS47L15_IRQ1_STATUS_1
    {0x20, CS47L15_IRQ_DSP1_BUS_ERR_EINT1_MASK , CS47L15_EVENT_FLAG_DSP_BUS_ERROR},    //< CS47L15_IRQ1_STATUS_33
    {0x0A, CS47L15_DSP_IRQ1_EINT1_MASK         , CS47L15_EVENT_FLAG_DSP_IRQ1},         //< CS47L15_IRQ1_STATUS_11
    {0x0E, CS47L15_SPK_OVERHEAT_WARN_EINT1_MASK, CS47L15_EVENT_FLAG_OVERTEMP_WARNING}, //< CS47L15_IRQ1_STATUS_15
    {0x0E, CS47L15_SPK_OVERHEAT_EINT1_MASK     , CS47L15_EVENT_FLAG_OVERTEMP_ERROR},   //< CS47L15_IRQ1_STATUS_15
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
#ifdef CS47L15_USEFUL_UNUSED
/**
 * Find if an algorithm is in the algorithm list and return true if it is.
 * Returns false if not.
 */
static bool find_algid(fw_img_info_t *fw_info, uint32_t algid_id)
{
    if (fw_info)
    {
        for (uint32_t i = 0; i < fw_info->header.alg_id_list_size; i++)
        {
            if (fw_info->alg_id_list[i] == algid_id)
                return true;
        }
    }

    return false;
}

bool cs47l15_find_algid(cs47l15_t *driver, uint32_t dsp_core, uint32_t algid_id)
{
    bool ret;

    if (dsp_core > CS47L15_NUM_DSP)
        return false;

    if (dsp_core != 0)
    {
        return find_algid(driver->dsp_info[dsp_core - 1].fw_info, algid_id);
    }
    else
    {
        // search all DSPs if dsp_core is 0
        for (uint32_t i = 0; i < CS47L15_NUM_DSP; i++)
        {
            ret = find_algid(driver->dsp_info[i].fw_info, algid_id);

            if (ret)
                return true;
        }
    }

    return false;
}
#endif

/**
 * Find if a symbol is in the symbol table and return its address if it is.
 *
 * This will search through the symbol table pointed to in the 'fw_info' member of the driver state and return
 * the control port register address to use for access.  The 'symbol_id' parameter must be from the group CS47L15_SYM_.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] symbol_id        id of symbol to search for
 *
 * @return
 * - non-0 - symbol register address
 * - 0 - symbol not found.
 *
 */
static uint32_t find_symbol(fw_img_info_t *fw_info, uint32_t symbol_id)
{
    if (fw_info)
    {
        for (uint32_t i = 0; i < fw_info->header.sym_table_size; i++)
        {
            if (fw_info->sym_table[i].sym_id == symbol_id)
                return fw_info->sym_table[i].sym_addr;
        }
    }

    return 0;
}

uint32_t cs47l15_find_symbol(cs47l15_t *driver, uint32_t dsp_core, uint32_t symbol_id)
{
    uint32_t ret;

    if (dsp_core > CS47L15_NUM_DSP)
        return false;

    if (dsp_core != 0)
    {
        return find_symbol(driver->dsp_info[dsp_core - 1].fw_info, symbol_id);
    }
    else
    {
        // search all DSPs if dsp_core is 0
        for (uint32_t i = 0; i < CS47L15_NUM_DSP; i++)
        {
            ret = find_symbol(driver->dsp_info[i].fw_info, symbol_id);

            if (ret)
                return ret;
        }
    }

    return 0;
}

/**
 * Writes from byte array to consecutive number of Control Port memory addresses
 *
 * This control port call only supports non-blocking calls.  This function also only supports I2C transactions.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be read
 * @param [in] bytes            pointer to array of bytes to write via Control Port bus
 * @param [in] length           number of bytes to write
 *
 * @return
 * - CS47L15_STATUS_FAIL        if the call to BSP failed
 * - CS47L15_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
static uint32_t cs47l15_cp_bulk_write_block(cs47l15_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length)
{
    uint32_t ret = CS47L15_STATUS_OK;
    uint32_t bsp_status;
    cs47l15_bsp_config_t *b = &(driver->config.bsp_config);
    uint8_t write_buffer[4];

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port
     * transaction.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_status = bsp_driver_if_g->spi_write(b->bsp_dev_id,
                                            &write_buffer[0],
                                            4,
                                            bytes,
                                            length);

    if (bsp_status == BSP_STATUS_FAIL)
    {
        ret = CS47L15_STATUS_FAIL;
    }

    return ret;
}

/**
 * Notify the driver when the CS47L15 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS47L15_MODE_HANDLING_CONTROLS to
 * CS47L15_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs47l15_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */
static void cs47l15_irq_callback(uint32_t status, void *cb_arg)
{
    cs47l15_t *d;

    d = (cs47l15_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS47L15_MODE_HANDLING_EVENTS
        d->mode = CS47L15_MODE_HANDLING_EVENTS;
    }

    return;
}

/**
 * Reads the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be read
 * @param [out] val             Pointer to where the read register value will be stored
 *
 * @return
 * - CS47L15_STATUS_FAIL        if the call to BSP failed
 * - CS47L15_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs47l15_read_reg(cs47l15_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret = CS47L15_STATUS_FAIL;
    cs47l15_bsp_config_t *b = &(driver->config.bsp_config);
    uint32_t read_size;
    uint8_t write_buffer[4];
    uint8_t read_buffer[4];

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port transaction.
     * Since register address is first written, write_buffer is filled with register address.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    // Currently only SPI transactions are supported
    if (b->bus_type == CS47L15_BUS_TYPE_SPI)
    {
        uint32_t bsp_status;

        // Registers below 0x3000 are 16-bit, all others are 32-bit
        if (addr < 0x3000)
            read_size = 2;
        else
            read_size = 4;

        bsp_status = bsp_driver_if_g->spi_read(b->bsp_dev_id,
                                               &write_buffer[0],
                                               4,
                                               &read_buffer[0],
                                               read_size);
        if (BSP_STATUS_OK == bsp_status)
        {
            /*
             * Switch from Big-Endian format required for Control Port transaction to Little-Endian contents of
             * uint32_t 'val'
             *
             * FIXME: This is not platform independent.
             */
            if (read_size == 2)
            {
                *val = 0;
                ADD_BYTE_TO_WORD(*val, read_buffer[0], 1);
                ADD_BYTE_TO_WORD(*val, read_buffer[1], 0);
            }
            else
            {
                ADD_BYTE_TO_WORD(*val, read_buffer[0], 3);
                ADD_BYTE_TO_WORD(*val, read_buffer[1], 2);
                ADD_BYTE_TO_WORD(*val, read_buffer[2], 1);
                ADD_BYTE_TO_WORD(*val, read_buffer[3], 0);
            }

            ret = CS47L15_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Writes the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be written
 * @param [in] val              32-bit value to be written
 *
 * @return
 * - CS47L15_STATUS_FAIL        if the call to BSP failed
 * - CS47L15_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs47l15_write_reg(cs47l15_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret = CS47L15_STATUS_FAIL;
    uint32_t bsp_status = BSP_STATUS_OK;
    cs47l15_bsp_config_t *b = &(driver->config.bsp_config);
    uint32_t write_size;
    uint8_t write_buffer[8];

    // Registers below 0x3000 are 16-bit, all others are 32-bit
    if (addr < 0x3000)
        write_size = 2;
    else
        write_size = 4;
    /*
     * Copy Little-Endian contents of 'addr' and 'val' to the Big-Endian format required for Control Port transactions
     * using a uint8_t write_buffer.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    if (write_size == 2)
    {
        write_buffer[4] = GET_BYTE_FROM_WORD(val, 1);
        write_buffer[5] = GET_BYTE_FROM_WORD(val, 0);
    }
    else
    {
        write_buffer[4] = GET_BYTE_FROM_WORD(val, 3);
        write_buffer[5] = GET_BYTE_FROM_WORD(val, 2);
        write_buffer[6] = GET_BYTE_FROM_WORD(val, 1);
        write_buffer[7] = GET_BYTE_FROM_WORD(val, 0);
    }

    // Currently only SPI transactions are supported
    if (b->bus_type == CS47L15_BUS_TYPE_SPI)
    {
        bsp_status = bsp_driver_if_g->spi_write(b->bsp_dev_id,
                                                &write_buffer[0],
                                                4,
                                                &write_buffer[4],
                                                write_size);
    }

    if (BSP_STATUS_OK == bsp_status)
    {
        ret = CS47L15_STATUS_OK;
    }

    return ret;
}

uint32_t cs47l15_update_reg(cs47l15_t *driver, uint32_t addr, uint32_t mask, uint32_t val)
{
    uint32_t tmp, ret, orig;

    ret = cs47l15_read_reg(driver, addr, &orig);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    tmp = (orig & ~mask) | val;

    if (tmp != orig)
    {
        ret = cs47l15_write_reg(driver, addr, tmp);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
    }

    return CS47L15_STATUS_OK;
}

/**
 * Writes the contents of a single register/memory address that ACK's with a default value
 *
 * This performs the same function as cs47l15_write_reg, with the addition of, after writing the value to the address
 * specified, will periodically read back the register and verify that a default value is restored (0)
 * indicating the write succeeded.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be written
 * @param [in] val              32-bit value to be written
 *
 * @return
 * - CS47L15_STATUS_FAIL        if the call to BSP failed or if register is never restored to 0
 * - CS47L15_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs47l15_write_acked_reg(cs47l15_t *driver, uint32_t addr, uint32_t val)
{
    int count;
    uint32_t temp_val;
    cs47l15_write_reg(driver, addr, val);

    for (count = 0 ; count < CS47L15_POLL_ACK_CTRL_MAX; count++)
    {
        bsp_driver_if_g->set_timer(CS47L15_POLL_ACK_CTRL_MS, NULL, NULL);

        cs47l15_read_reg(driver, addr, &temp_val);
        if (temp_val == 0)
        {
            return CS47L15_STATUS_OK;
        }
    }
    return CS47L15_STATUS_FAIL;
}

/**
 * Power up from Standby
 *
 * This function performs all necessary steps to enable the DSP core.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS47L15_STATUS_FAIL if:
 *      - OTP_BOOT_DONE is not set
 *      - DSP Scratch register is not cleared
 * - CS47L15_STATUS_OK          otherwise
 *
 */
static uint32_t cs47l15_power_up(cs47l15_t *driver, cs47l15_dsp_t *dsp_info)
{
    uint32_t ret;

    // Lock Region 1
    ret = cs47l15_write_reg(driver,
                            dsp_info->base_addr + CS47L15_DSP_OFF_REGION_LOCK_1_0,
                            CS47L15_REGION_LOCK_CODE0 << CS47L15_REGION_LOCK_UPPER_SHIFT);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver,
                            dsp_info->base_addr + CS47L15_DSP_OFF_REGION_LOCK_1_0,
                            CS47L15_REGION_LOCK_CODE1 << CS47L15_REGION_LOCK_UPPER_SHIFT);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    // Lock Regions 2 & 3
    ret = cs47l15_write_reg(driver,
                            dsp_info->base_addr + CS47L15_DSP_OFF_REGION_LOCK_3_2,
                            CS47L15_REGION_LOCK_CODE0 << CS47L15_REGION_LOCK_UPPER_SHIFT | CS47L15_REGION_LOCK_CODE0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver,
                            dsp_info->base_addr + CS47L15_DSP_OFF_REGION_LOCK_3_2,
                            CS47L15_REGION_LOCK_CODE1 << CS47L15_REGION_LOCK_UPPER_SHIFT | CS47L15_REGION_LOCK_CODE1);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_update_reg(driver,
                             dsp_info->base_addr + CS47L15_DSP_OFF_CONFIG_1,
                             CS47L15_DSP1_CORE_ENA_MASK | CS47L15_DSP1_START_MASK,
                             CS47L15_DSP1_CORE_ENA | CS47L15_DSP1_START);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    return CS47L15_STATUS_OK;
}

/**
 * Power down to Standby
 *
 * This function performs all necessary steps to disable the ADSP2 core on the CS47L15
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS47L15_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Firmware control addresses cannot be resolved by Symbol ID
 * - CS47L15_STATUS_OK          otherwise
 *
 */
static uint32_t cs47l15_power_down(cs47l15_t *driver, cs47l15_dsp_t *dsp_info)
{
    uint32_t ret;

    // Stop Watchdog Timer
    ret = cs47l15_update_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_WATCHDOG_1, CS47L15_DSP1_WDT_ENA, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    // Disable DSP
    ret = cs47l15_update_reg(driver,
                             dsp_info->base_addr + CS47L15_DSP_OFF_CONFIG_1,
                             CS47L15_DSP1_CORE_ENA_MASK | CS47L15_DSP1_START_MASK,
                             0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_3, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_1, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_2, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    return CS47L15_STATUS_OK;
}

/**
 * Memory enable
 *
 * This function performs all necessary steps to enable the memory of the DSP core on the CS47L15
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS47L15_STATUS_FAIL if:
 *      - Control port activity fails
 * - CS47L15_STATUS_OK          otherwise
 *
 */
static uint32_t cs47l15_power_mem_ena(cs47l15_t *driver, cs47l15_dsp_t *dsp_info)
{
    uint32_t val, i, ret;

    ret = cs47l15_update_reg(driver, CS47L15_DSP_CLOCK_1, CS47L15_DSP_CLK_ENA_MASK, CS47L15_DSP_CLK_ENA);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_read_reg(driver, CS47L15_DSP_CLOCK_2, &val);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_update_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_CONFIG_2, CS47L15_DSP1_CLK_FREQ_MASK,  val);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_CONFIG_1, CS47L15_DSP1_MEM_ENA);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    for (i = 0; i < CS47L15_POLL_MEM_ENA_MAX; i++)
    {
        ret = cs47l15_read_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_STATUS_1, &val);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }

        if (val & CS47L15_DSP1_RAM_RDY)
            break;

        bsp_driver_if_g->set_timer(CS47L15_POLL_MEM_ENA_MS, NULL, NULL);
    }

    if (i == CS47L15_POLL_MEM_ENA_MAX)
    {
        return CS47L15_STATUS_FAIL;
    }

    return CS47L15_STATUS_OK;
}

/**
 * Memory disable
 *
 * This function performs all necessary steps to disable the memory of the DSP core on the CS47L15.  After
 * calling this function, the contents of DSP memory will be lost.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS47L15_STATUS_FAIL if:
 *      - Control port activity fails
 * - CS47L15_STATUS_OK          otherwise
 *
 */
static uint32_t cs47l15_power_mem_dis(cs47l15_t *driver, cs47l15_dsp_t *dsp_info)
{
    return cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_CONFIG_1, CS47L15_DSP1_MEM_ENA);
}

/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs47l15_notification_callback_t).
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS47L15_STATUS_FAIL         Control port activity fails
 * - CS47L15_STATUS_OK          otherwise
 *
 * @see CS47L15_EVENT_FLAG_
 * @see cs47l15_notification_callback_t
 *
 */
static uint32_t cs47l15_event_handler(cs47l15_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    uint32_t old_reg = 0;
    uint32_t new_reg;

    driver->event_flags = 0;

    for (uint32_t i = 0; i < (sizeof(cs47l15_event_data) / sizeof(irq_reg_t)); i++)
    {
        new_reg = CS47L15_IRQ1_STATUS_1 + cs47l15_event_data[i].irq_reg_offset;
        if (old_reg != new_reg)
        {
            ret = cs47l15_read_reg(driver, new_reg, &temp_reg_val);
            if (ret)
            {
                return ret;
            }
        }
        old_reg = new_reg;

        if (temp_reg_val & cs47l15_event_data[i].mask)
        {
            driver->event_flags |= cs47l15_event_data[i].event_flag;
            ret = cs47l15_write_reg(driver, CS47L15_IRQ1_STATUS_1 + cs47l15_event_data[i].irq_reg_offset,\
                                    cs47l15_event_data[i].mask);
        }
    }

    return CS47L15_STATUS_OK;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs47l15_initialize(cs47l15_t *driver)
{
    uint32_t ret = CS47L15_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs47l15_t));

        ret = CS47L15_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs47l15_configure(cs47l15_t *driver, cs47l15_config_t *config)
{
    uint32_t ret = CS47L15_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.bsp_int_gpio_id,
                                                &cs47l15_irq_callback,
                                                driver);

        if (ret == BSP_STATUS_FAIL)
        {
            return ret;
        }

        // Configure DSP Core 1
        driver->dsp_info[0].dsp_core = 1;
        driver->dsp_info[0].base_addr = 0xFFE00;

        // Advance driver to CONFIGURED state
        driver->state = CS47L15_STATE_CONFIGURED;
    }

    return ret;
}

/**
 * Processes driver events and notifications
 *
 */
uint32_t cs47l15_process(cs47l15_t *driver)
{
    // check for driver state
    if ((driver->state != CS47L15_STATE_UNCONFIGURED) && (driver->state != CS47L15_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS47L15_MODE_HANDLING_EVENTS)
        {
            // Check for valid state to process events
            if ((driver->state == CS47L15_STATE_STANDBY))
            {
                // run through event handler
                if (CS47L15_STATUS_OK == cs47l15_event_handler(driver))
                {
                    driver->mode = CS47L15_MODE_HANDLING_CONTROLS;
                }
                else
                {
                    return CS47L15_STATUS_FAIL;
                }
            }
            // If in invalid state for handling events (i.e. BHM, Calibration), simply switch back to Handling Controls
            else
            {
                driver->mode = CS47L15_MODE_HANDLING_CONTROLS;
            }
        }

        if (driver->event_flags)
        {
            if (driver->config.bsp_config.notification_cb != NULL)
            {
                driver->config.bsp_config.notification_cb(driver->event_flags,
                                                          driver->config.bsp_config.notification_cb_arg);
            }

            driver->event_flags = 0;
        }
    }
    return CS47L15_STATUS_OK;
}

/**
 * Reset the CS47L15
 *
 */
uint32_t cs47l15_reset(cs47l15_t *driver)
{
    uint32_t temp_reg_val;
    uint32_t ret;
    uint32_t iter_timeout = 0;

    // Ensure DCVDD is disabled
    bsp_driver_if_g->set_supply(driver->config.bsp_config.bsp_dcvdd_supply_id, BSP_SUPPLY_DISABLE);
    bsp_driver_if_g->set_timer(2, NULL, NULL);

    // Drive RESET high
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.bsp_reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(2, NULL, NULL);

    // Enable DCVDD with RESET high (deasserted)
    bsp_driver_if_g->set_supply(driver->config.bsp_config.bsp_dcvdd_supply_id, BSP_SUPPLY_ENABLE);
    bsp_driver_if_g->set_timer(10, NULL, NULL);

    do
    {
        //wait for boot sequence to finish
        ret = cs47l15_read_reg(driver, CS47L15_IRQ1_RAW_STATUS_1, &temp_reg_val);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
        bsp_driver_if_g->set_timer(10, NULL, NULL);
        iter_timeout++;
        if (iter_timeout > 20)
        {
            return CS47L15_STATUS_FAIL;
        }
    } while (!(temp_reg_val & CS47L15_BOOT_DONE_STS1_MASK));

    // Read device ID and revision ID
    ret = cs47l15_read_reg(driver, CS47L15_SOFTWARE_RESET, &temp_reg_val);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }
    driver->devid = temp_reg_val;

    ret = cs47l15_read_reg(driver, CS47L15_HARDWARE_REVISION, &temp_reg_val);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }
    driver->revid = temp_reg_val;

    // Apply errata
    for (uint32_t i = 0; i < (sizeof(cs47l15_reva_errata_patch) / sizeof(uint32_t)); i+=2)
    {
        ret = cs47l15_write_reg(driver, cs47l15_reva_errata_patch[i], cs47l15_reva_errata_patch[i+1]);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
    }

    // Write configuration data
    // Since cs47l15 is configured via WISCE scripts, can write without masking individual bits
    for (uint32_t count = 0; count < driver->config.syscfg_regs_total; count++)
    {
        ret = cs47l15_write_reg(driver, driver->config.syscfg_regs[count].address, driver->config.syscfg_regs[count].value);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
    }

    // Unmask interrupts
    // Omit first mask register, as BOOT_DONE_EINT1 is enabled by default
    for (uint32_t i = 1; i < (sizeof(cs47l15_event_data) / sizeof(irq_reg_t)); i++)
    {
        ret = cs47l15_update_reg(driver, CS47L15_IRQ1_MASK_1 + cs47l15_event_data[i].irq_reg_offset, cs47l15_event_data[i].mask, 0);
        if (ret)
        {
            return ret;
        }
    }

    driver->state = CS47L15_STATE_STANDBY;

    return CS47L15_STATUS_OK;
}

/**
 * Write block of data to the CS47L15 register file
 *
 */
uint32_t cs47l15_write_block(cs47l15_t *driver, uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr == 0 || data == NULL || size == 0 || size % 2 != 0)
    {
        return CS47L15_STATUS_FAIL;
    }

    return cs47l15_cp_bulk_write_block(driver, addr, data, size);
}

/**
 * Finish booting the CS47L15
 *
 */
uint32_t cs47l15_boot(cs47l15_t *driver, uint32_t dsp_core, fw_img_info_t *fw_info)
{
    cs47l15_dsp_t *dsp_info;
    uint32_t ret;

    if (dsp_core > CS47L15_NUM_DSP || dsp_core == 0)
        return CS47L15_STATUS_FAIL;

    dsp_info = &driver->dsp_info[dsp_core - 1];
    dsp_info->fw_info = fw_info;

    if (fw_info == NULL)
    {
        return CS47L15_STATUS_OK;
    }

    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_3, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }
    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_1, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }
    ret = cs47l15_write_reg(driver, dsp_info->base_addr + CS47L15_DSP_OFF_DMA_CONFIG_2, 0);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    return CS47L15_STATUS_OK;
}

/**
 * Change the power state
 *
 */
uint32_t cs47l15_power(cs47l15_t *driver, uint32_t dsp_core, uint32_t power_state)
{
    uint32_t ret = CS47L15_STATUS_OK;
    cs47l15_dsp_t *dsp_info;

    if (dsp_core > CS47L15_NUM_DSP || dsp_core == 0)
        return CS47L15_STATUS_FAIL;

    dsp_info = &driver->dsp_info[dsp_core - 1];

    switch (power_state)
    {
        case CS47L15_POWER_MEM_ENA:
            ret = cs47l15_power_mem_ena(driver, dsp_info);

            break;

        case CS47L15_POWER_UP:
            ret = cs47l15_power_up(driver, dsp_info);

            break;

        case CS47L15_POWER_DOWN:
            ret = cs47l15_power_down(driver, dsp_info);

            break;

        case CS47L15_POWER_MEM_DIS:
            ret = cs47l15_power_mem_dis(driver, dsp_info);

            break;

        default:
            ret = CS47L15_STATUS_FAIL;
            break;
    }

    return ret;
}

/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS47L15 Smart
 * Codec Driver.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS47L15 MCU Driver Software Package to integrate the CS47L15 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS47L15 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS47L15.  This guide should be used along with the CS47L15 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS47L15 system integration, please contact your Cirrus Logic Representative.
 */
