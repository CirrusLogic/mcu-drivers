/**
 * @file cs47l15.c
 *
 * @brief The CS47L15 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020-2021 All Rights Reserved, http://www.cirrus.com/
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

/**
 * FLL defines
 */

#define CS47L15_FLL_MAX_FREF            (13500000)
#define CS47L15_FLL_MIN_FOUT            (90000000)
#define CS47L15_FLL_MAX_FOUT            (100000000)
#define CS47L15_FLL_MAX_FRATIO          (16)
#define CS47L15_FLL_MAX_REFDIV          (8)
#define CS47L15_FLL_MAX_N               (1023)

#define CS47L15_FLL_SYNCHRONISER_OFFS       (0x10)
#define CS47L15_FLL_CONTROL_1_OFFS          (0x1)
#define CS47L15_FLL_CONTROL_2_OFFS          (0x2)
#define CS47L15_FLL_CONTROL_3_OFFS          (0x3)
#define CS47L15_FLL_CONTROL_4_OFFS          (0x4)
#define CS47L15_FLL_CONTROL_5_OFFS          (0x5)
#define CS47L15_FLL_CONTROL_6_OFFS          (0x6)
#define CS47L15_FLL_CONTROL_7_OFFS          (0x9)
#define CS47L15_FLL_EFS_2_OFFS              (0xA)
#define CS47L15_FLL_SYNCHRONISER_1_OFFS     (0x1)
#define CS47L15_FLL_SYNCHRONISER_7_OFFS     (0x7)

#define CS47L15_FLLAO_CONTROL_1_OFFS        (0x1)
#define CS47L15_FLLAO_CONTROL_2_OFFS        (0x2)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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

static const struct {
    uint32_t min;
    uint32_t max;
    uint16_t  fratio;
    int32_t  ratio;
} fll_sync_fratios[] = {
    {       0,    64000, 4, 16 },
    {   64000,   128000, 3,  8 },
    {  128000,   256000, 2,  4 },
    {  256000,  1000000, 1,  2 },
    { 1000000, 13500000, 0,  1 },
};

struct cs47l15_fll_gains {
    uint32_t  min;
    uint32_t  max;
    int32_t   gain;            /* main gain */
    int32_t   alt_gain;        /* alternate integer gain */
};

static const struct cs47l15_fll_gains cs47l15_fll_sync_gains[] = {
    {       0,   256000, 0, -1 },
    {  256000,  1000000, 2, -1 },
    { 1000000, 13500000, 4, -1 },
};

static const struct cs47l15_fll_gains cs47l15_fll_main_gains[] = {
    {       0,   100000, 0, 2 },
    {  100000,   375000, 2, 2 },
    {  375000,   768000, 3, 2 },
    {  768001,  1500000, 3, 3 },
    { 1500000,  6000000, 4, 3 },
    { 6000000, 13500000, 5, 3 },
};

struct reg_sequence {
    uint32_t reg;
    uint32_t def;
};

static const struct reg_sequence cs47l15_fll_ao_32K_49M_patch[] = {
    { CS47L15_FLL_AO_CONTROL_2,  0x02EE },
    { CS47L15_FLL_AO_CONTROL_3,  0x0000 },
    { CS47L15_FLL_AO_CONTROL_4,  0x0001 },
    { CS47L15_FLL_AO_CONTROL_5,  0x0002 },
    { CS47L15_FLL_AO_CONTROL_6,  0x8001 },
    { CS47L15_FLL_AO_CONTROL_7,  0x0004 },
    { CS47L15_FLL_AO_CONTROL_8,  0x0077 },
    { CS47L15_FLL_AO_CONTROL_10, 0x06D8 },
    { CS47L15_FLL_AO_CONTROL_11, 0x0085 },
    { CS47L15_FLL_AO_CONTROL_2,  0x82EE },
};

static const struct reg_sequence cs47l15_fll_ao_32K_45M_patch[] = {
    { CS47L15_FLL_AO_CONTROL_2,  0x02B1 },
    { CS47L15_FLL_AO_CONTROL_3,  0x0001 },
    { CS47L15_FLL_AO_CONTROL_4,  0x0010 },
    { CS47L15_FLL_AO_CONTROL_5,  0x0002 },
    { CS47L15_FLL_AO_CONTROL_6,  0x8001 },
    { CS47L15_FLL_AO_CONTROL_7,  0x0004 },
    { CS47L15_FLL_AO_CONTROL_8,  0x0077 },
    { CS47L15_FLL_AO_CONTROL_10, 0x06D8 },
    { CS47L15_FLL_AO_CONTROL_11, 0x0005 },
    { CS47L15_FLL_AO_CONTROL_2,  0x82B1 },
};

struct cs47l15_fllao_patch {
    uint32_t fin;
    uint32_t fout;
    const struct reg_sequence *patch;
    uint32_t patch_size;
};

static const struct cs47l15_fllao_patch cs47l15_fllao_settings[] = {
    {
        .fin = 32768,
        .fout = 49152000,
        .patch = cs47l15_fll_ao_32K_49M_patch,
        .patch_size = ARRAY_SIZE(cs47l15_fll_ao_32K_49M_patch),

    },
    {
        .fin = 32768,
        .fout = 45158400,
        .patch = cs47l15_fll_ao_32K_45M_patch,
        .patch_size = ARRAY_SIZE(cs47l15_fll_ao_32K_45M_patch),
    },
};

struct cs47l15_fll_cfg {
    int32_t n;
    uint32_t theta;
    uint32_t lambda;
    int32_t refdiv;
    int32_t fratio;
    int32_t gain;
    int32_t alt_gain;
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

uint32_t cs47l15_find_symbol(cs47l15_t *driver, uint32_t dsp_core, uint32_t symbol_id)
{
    uint32_t ret;

    if (dsp_core > CS47L15_NUM_DSP)
        return false;

    if (dsp_core != 0)
    {
        return fw_img_find_symbol(driver->dsp_info[dsp_core - 1].fw_info, symbol_id);
    }
    else
    {
        // search all DSPs if dsp_core is 0
        for (uint32_t i = 0; i < CS47L15_NUM_DSP; i++)
        {
            ret = fw_img_find_symbol(driver->dsp_info[i].fw_info, symbol_id);

            if (ret)
                return ret;
        }
    }

    return 0;
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
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }

    return CS47L15_STATUS_OK;
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
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }

    return CS47L15_STATUS_OK;
}

uint32_t cs47l15_update_reg(cs47l15_t *driver, uint32_t addr, uint32_t mask, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_update_reg(cp, addr, mask, val);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
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

static uint32_t cs47l15_write_fll(cs47l15_t *driver, uint32_t base,
                                  struct cs47l15_fll_cfg *cfg, int32_t source,
                                  bool sync, int32_t gain)
{
    uint32_t ret = CS47L15_STATUS_OK;

    ret = cs47l15_write_reg(driver, base + CS47L15_FLL_CONTROL_3_OFFS, cfg->theta);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver, base + CS47L15_FLL_CONTROL_4_OFFS, cfg->lambda);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver,
                            base + CS47L15_FLL_CONTROL_5_OFFS,
                            cfg->fratio << CS47L15_FLL1_FRATIO_SHIFT);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver,
                            base + CS47L15_FLL_CONTROL_6_OFFS,
                            cfg->refdiv << CS47L15_FLL1_REFCLK_DIV_SHIFT |
                            source << CS47L15_FLL1_REFCLK_SRC_SHIFT);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    if (sync)
    {
        ret = cs47l15_update_reg(driver,
                                 base + CS47L15_FLL_SYNCHRONISER_7_OFFS,
                                 CS47L15_FLL1_GAIN_MASK,
                                 gain << CS47L15_FLL1_GAIN_SHIFT);
    }
    else
    {
        ret = cs47l15_write_reg(driver,
                                base + CS47L15_FLL_CONTROL_7_OFFS,
                                gain << CS47L15_FLL1_GAIN_SHIFT);
    }
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    ret = cs47l15_write_reg(driver,
                            base + CS47L15_FLL_CONTROL_2_OFFS,
                            CS47L15_FLL1_CTRL_UPD | cfg->n);

    return ret;
}

static int32_t cs47l15_find_sync_fratio(uint32_t fref, int32_t  *fratio)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(fll_sync_fratios); i++)
    {
        if (fll_sync_fratios[i].min <= fref &&
            fref <= fll_sync_fratios[i].max)
            {
            if (fratio)
            {
                *fratio = fll_sync_fratios[i].fratio;
            }
            return fll_sync_fratios[i].ratio;
        }
    }

    return -1;
}

static int32_t cs47l15_find_main_fratio(uint32_t fref, uint32_t fout,
                                        int32_t *fratio)
{
    int32_t ratio = 1;

    while ((fout / (ratio * fref)) > CS47L15_FLL_MAX_N)
    {
        ratio++;
    }
    if (fratio)
    {
        *fratio = ratio - 1;
    }
    return ratio;
}

static int32_t cs47l15_find_fratio(cs47l15_fll_t *fll,
                                   uint32_t fref,
                                   bool sync, int32_t *fratio)
{
    if (sync)
    {
        return cs47l15_find_sync_fratio(fref, fratio);
    }
    else
    {
        return cs47l15_find_main_fratio(fref, fll->fout, fratio);
    }
}

static int32_t cs47l15_calc_fratio(cs47l15_fll_t *fll,
                                   struct cs47l15_fll_cfg *cfg,
                                   uint32_t fref,
                                   bool sync)
{
    int32_t init_ratio, div;

    /* fref must be <=13.5MHz, find initial refdiv */
    div = 1;
    cfg->refdiv = 0;
    while (fref > CS47L15_FLL_MAX_FREF)
    {
        div *= 2;
        fref /= 2;
        cfg->refdiv++;

        if (div > CS47L15_FLL_MAX_REFDIV)
        {
            return -1;  // return a neg value to signal an error
        }
    }

    /* Find an appropriate FLL_FRATIO */
    init_ratio = cs47l15_find_fratio(fll, fref, sync, &cfg->fratio);
    if (init_ratio < 0)
    {
        return init_ratio;
    }

    if (!sync)
    {
        cfg->fratio = init_ratio - 1;
    }

    return init_ratio;
}

static uint32_t cs47l15_find_fll_gain(struct cs47l15_fll_cfg *cfg,
                                      uint32_t fref,
                                      const struct cs47l15_fll_gains *gains,
                                      int32_t n_gains)
{
    int32_t i;

    for (i = 0; i < n_gains; i++)
    {
        if (gains[i].min <= fref && fref <= gains[i].max)
        {
            cfg->gain = gains[i].gain;
            cfg->alt_gain = gains[i].alt_gain;
            return CS47L15_STATUS_OK;
        }
    }
    return CS47L15_STATUS_FAIL;
}

static uint32_t gcd(uint32_t n1, uint32_t n2)
{
    while (n1 != n2)
    {
        if (n1 > n2)
        {
            n1 -= n2;
        }
        else
        {
            n2 -= n1;
        }
    }
    return n1;
}

static uint32_t cs47l15_calc_fll(cs47l15_fll_t *fll,
                                 struct cs47l15_fll_cfg *cfg,
                                 uint32_t fref,
                                 bool sync)
{
    uint32_t gcd_fll;
    const struct cs47l15_fll_gains *gains;
    int32_t n_gains;
    int32_t ratio;
    uint32_t ret;

    /* Find an appropriate FLL_FRATIO and refdiv */
    ratio = cs47l15_calc_fratio(fll, cfg, fref, sync);
    if (ratio < 0)
    {
        return CS47L15_STATUS_FAIL;
    }

    /* Apply the division for our remaining calculations */
    fref = fref / (1 << cfg->refdiv);

    cfg->n = fll->fout / (ratio * fref);

    if (fll->fout % (ratio * fref))
    {
        gcd_fll = gcd(fll->fout, ratio * fref);

        cfg->theta = (fll->fout - (cfg->n * ratio * fref))/gcd_fll;
        cfg->lambda = (ratio * fref) / gcd_fll;
    }
    else
    {
        cfg->theta = 0;
        cfg->lambda = 0;
    }

    /*
     * Round down to 16bit range with cost of accuracy lost.
     * Denominator must be bigger than numerator so we only
     * take care of it.
     */
    while (cfg->lambda >= (1 << 16)) {
        cfg->theta >>= 1;
        cfg->lambda >>= 1;
    }

    if (sync)
    {
        gains = cs47l15_fll_sync_gains;
        n_gains = ARRAY_SIZE(cs47l15_fll_sync_gains);
    }
    else
    {
        gains = cs47l15_fll_main_gains;
        n_gains = ARRAY_SIZE(cs47l15_fll_main_gains);
    }

    ret = cs47l15_find_fll_gain(cfg, fref, gains, n_gains);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    return CS47L15_STATUS_OK;
}

static uint32_t cs47l15_is_enabled_fll(cs47l15_t *driver, uint32_t base, bool *enabled)
{
    uint32_t reg;
    uint32_t ret;

    *enabled = false;
    ret = cs47l15_read_reg(driver, base + CS47L15_FLL_CONTROL_1_OFFS, &reg);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    if (reg & CS47L15_FLL1_ENA)
    {
        *enabled = true;
    }
    return CS47L15_STATUS_OK;
}

static uint32_t cs47l15_set_fll_phase_integrator(cs47l15_t* driver,
                                                 cs47l15_fll_t *fll,
                                                 struct cs47l15_fll_cfg *ref_cfg,
                                                 bool sync)
{
    uint32_t val, ret;

    if (!sync && ref_cfg->theta == 0)
    {
        val = (1 << CS47L15_FLL1_PHASE_ENA_SHIFT) |
              (2 << CS47L15_FLL1_PHASE_GAIN_SHIFT);
    }
    else
    {
        val = 2 << CS47L15_FLL1_PHASE_GAIN_SHIFT;
    }

    ret = cs47l15_update_reg(driver,
                             fll->base + CS47L15_FLL_EFS_2_OFFS,
                             CS47L15_FLL1_PHASE_ENA_MASK | CS47L15_FLL1_PHASE_GAIN_MASK,
                             val);

    return ret;
}

static void cs47l15_disable_fll(cs47l15_t* driver,
                                cs47l15_fll_t *fll)
{
    uint32_t sync_base = fll->base + CS47L15_FLL_SYNCHRONISER_OFFS;

    cs47l15_update_reg(driver,
                       fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                       CS47L15_FLL1_FREERUN_MASK, CS47L15_FLL1_FREERUN);

    cs47l15_update_reg(driver,
                        fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                        CS47L15_FLL1_ENA_MASK, 0);
    cs47l15_update_reg(driver,
                        sync_base + CS47L15_FLL_SYNCHRONISER_1_OFFS,
                        CS47L15_FLL1_SYNC_ENA_MASK, 0);

    cs47l15_update_reg(driver,
                       fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                       CS47L15_FLL1_FREERUN_MASK, 0);
}

static int32_t cs47l15_apply_config_fll(cs47l15_t* driver, cs47l15_fll_t *fll)
{
    bool have_sync = false;
    uint32_t sync_base;
    int32_t gain;
    uint32_t ret;
    bool already_enabled = false, sync_enabled = false;
    struct cs47l15_fll_cfg ref_cfg;

    ret = cs47l15_is_enabled_fll(driver, fll->base, &already_enabled);
    if (ret != CS47L15_STATUS_OK)
    {
        return ret;
    }

    if (fll->ref_src < 0 || fll->ref_freq == 0)
    {
        cs47l15_disable_fll(driver, fll);
        return CS47L15_STATUS_FAIL;
    }

    if (fll->fout < CS47L15_FLL_MIN_FOUT || fll->fout > CS47L15_FLL_MAX_FOUT)
    {
        cs47l15_disable_fll(driver, fll);
        return CS47L15_STATUS_FAIL;
    }

    sync_base = fll->base + CS47L15_FLL_SYNCHRONISER_OFFS;
    ret = cs47l15_is_enabled_fll(driver, sync_base, &sync_enabled);
    if (ret == CS47L15_STATUS_FAIL)
    {
        return ret;
    }

    if (already_enabled)
    {
        // Facilitate smooth refclk across the transition
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                                 CS47L15_FLL1_FREERUN_MASK, CS47L15_FLL1_FREERUN);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }

        bsp_driver_if_g->set_timer(1, NULL, NULL);

        ret = cs47l15_write_reg(driver,
                                fll->base + CS47L15_FLL_CONTROL_7_OFFS,
                                0);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
    }

    /* Apply SYNCCLK setting */
    if (fll->sync_src >= 0)
    {
        ret = cs47l15_calc_fll(fll, &ref_cfg, fll->sync_freq, true);
        if (ret == CS47L15_STATUS_FAIL)
        {
            cs47l15_disable_fll(driver, fll);
            return ret;
        }

        ret = cs47l15_write_fll(driver,
                                sync_base,
                                &ref_cfg, fll->sync_src,
                                true,
                                ref_cfg.gain);
        if (ret == CS47L15_STATUS_FAIL)
        {
            cs47l15_disable_fll(driver, fll);
            return ret;
        }
        have_sync = true;
    }

    /* Apply REFCLK setting */
    ret = cs47l15_calc_fll(fll, &ref_cfg, fll->ref_freq, false);
    if (ret == CS47L15_STATUS_FAIL)
    {
        cs47l15_disable_fll(driver, fll);
        return ret;
    }

    /* Ref path hardcodes lambda to 65536 when sync is on */
    if (have_sync && ref_cfg.lambda)
    {
        ref_cfg.theta = (ref_cfg.theta * (1 << 16)) / ref_cfg.lambda;
    }

    ret = cs47l15_set_fll_phase_integrator(driver, fll, &ref_cfg, have_sync);
    if (ret == CS47L15_STATUS_FAIL)
    {
        cs47l15_disable_fll(driver, fll);
        return ret;
    }

    if (!have_sync && ref_cfg.theta == 0)
    {
        gain = ref_cfg.alt_gain;
    }
    else
    {
        gain = ref_cfg.gain;
    }

    ret = cs47l15_write_fll(driver, fll->base,
                            &ref_cfg, fll->ref_src,
                            false, gain);
    if (ret == CS47L15_STATUS_FAIL)
    {
        cs47l15_disable_fll(driver, fll);
        return ret;
    }

    /*
     * Increase the bandwidth if we're not using a low frequency
     * sync source.
     */
    if (have_sync && fll->sync_freq > 100000)
    {
        ret = cs47l15_update_reg(driver,
                                 sync_base + CS47L15_FLL_SYNCHRONISER_7_OFFS,
                                 CS47L15_FLL1_SYNC_DFSAT_MASK, 0);
    }
    else
    {
        ret = cs47l15_update_reg(driver,
                                 sync_base + CS47L15_FLL_SYNCHRONISER_7_OFFS,
                                 CS47L15_FLL1_SYNC_DFSAT_MASK,
                                 CS47L15_FLL1_SYNC_DFSAT);
    }
    if (ret == CS47L15_STATUS_FAIL)
    {
        cs47l15_disable_fll(driver, fll);
        return ret;
    }

    if (already_enabled)
    {
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                                 CS47L15_FLL1_FREERUN_MASK, 0);
    }

    return ret;
}

static uint32_t cs47l15_apply_config_fll_ao(cs47l15_t* driver,
                                            cs47l15_fll_t *fll,
                                            const struct reg_sequence *patch,
                                            uint32_t patch_size)
{
    uint32_t ret;
    bool already_enabled = false;
    uint32_t val;
    uint32_t  i;

    ret = cs47l15_is_enabled_fll(driver, fll->base, &already_enabled);
    if (ret != CS47L15_STATUS_OK)
    {
        return ret;
    }

    /* FLL_AO_HOLD must be set before configuring any registers */
    ret = cs47l15_update_reg(driver,
                             fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                             CS47L15_FLL_AO_HOLD_MASK, CS47L15_FLL_AO_HOLD);
    if (ret != CS47L15_STATUS_OK)
    {
        return ret;
    }

    for (i = 0; i < patch_size; i++)
    {
        val = patch[i].def;
        /* modify the patch to apply fll->ref_src as input clock */
        if (patch[i].reg == CS47L15_FLL_AO_CONTROL_6)
        {
            val &= ~CS47L15_FLL_AO_REFCLK_SRC_MASK;
            val |= (fll->ref_src << CS47L15_FLL_AO_REFCLK_SRC_SHIFT)
                   & CS47L15_FLL_AO_REFCLK_SRC_MASK;
        }

        ret = cs47l15_write_reg(driver, patch[i].reg, val);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }
    }

    /* Release the hold so that fll_ao locks to external frequency */
    ret = cs47l15_update_reg(driver,
                             fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                             CS47L15_FLL_AO_HOLD, 0);

    return ret;
}

static void cs47l15_disable_fll_ao(cs47l15_t* driver, cs47l15_fll_t *fll)
{
    cs47l15_update_reg(driver,
                       fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                       CS47L15_FLL_AO_HOLD_MASK, CS47L15_FLL_AO_HOLD);

    cs47l15_update_reg(driver,
                       fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                       CS47L15_FLL_AO_ENA_MASK, 0);
}

static int32_t cs47l15_set_fll_ao_refclk(cs47l15_t* driver,
                                         cs47l15_fll_t *fll, int32_t source,
                                         uint32_t fin, uint32_t fout)
{
    int32_t ret = 0;
    const struct reg_sequence *patch = NULL;
    uint32_t patch_size = 0;
    uint32_t i;

    if (fll->ref_src == source &&
        fll->ref_freq == fin && fll->fout == fout)
    {
        return CS47L15_STATUS_OK;
    }

    if (fll->ref_freq != fin || fll->fout != fout)
    {
        for (i = 0; i < ARRAY_SIZE(cs47l15_fllao_settings); i++)
        {
            if (cs47l15_fllao_settings[i].fin == fin &&
                cs47l15_fllao_settings[i].fout == fout)
            {
                break;
            }
        }

        if (i == ARRAY_SIZE(cs47l15_fllao_settings))
        {
            return CS47L15_STATUS_FAIL;
        }

        patch = cs47l15_fllao_settings[i].patch;
        patch_size = cs47l15_fllao_settings[i].patch_size;
    }

    fll->ref_src = source;
    fll->ref_freq = fin;
    fll->fout = fout;

    ret = cs47l15_apply_config_fll_ao(driver, fll, patch, patch_size);

    return ret;
}

static int32_t cs47l15_set_fll_syncclk(cs47l15_t *driver, cs47l15_fll_t *fll,
                                       int32_t source, uint32_t fref)
{
    if (fll->sync_src == source && fll->sync_freq == fref)
    {
        return CS47L15_STATUS_OK;
    }

    fll->sync_src = source;
    fll->sync_freq = fref;

    // We need refclk to be configured before applying syncclk
    if (fll->ref_src < 0 || fll->ref_freq == 0)
    {
        return CS47L15_STATUS_OK;
    }

    return cs47l15_apply_config_fll(driver, fll);
}

static int32_t cs47l15_set_fll_refclk(cs47l15_t *driver,
                                      cs47l15_fll_t *fll, int32_t source,
                                      uint32_t fref, uint32_t fout)
{
    int32_t ret;
    bool enabled;

    if (fll->ref_src == source &&
        fll->ref_freq == fref && fll->fout == fout)
    {
        return CS47L15_STATUS_OK;
    }

    // Prevent changing fout if FLL enabled
    if (fout != fll->fout)
    {
        ret = cs47l15_is_enabled_fll(driver, fll->base, &enabled);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
        if (enabled)
        {
            return CS47L15_STATUS_FAIL;
        }
    }

    fll->ref_src = source;
    fll->ref_freq = fref;
    fll->fout = fout;

    return cs47l15_apply_config_fll(driver, fll);
}

static uint32_t cs47l15_fll_init(cs47l15_t *driver,
                                 uint32_t fll_id)
{
    uint32_t ret = CS47L15_STATUS_OK;

    cs47l15_fll_t *fll = &driver->fll[fll_id];

    fll->id = fll_id;
    fll->ref_src = CS47L15_FLL_SRC_NONE;
    fll->sync_src = CS47L15_FLL_SRC_NONE;
    switch (fll_id)
    {
    case CS47L15_FLL1:
        fll->base = CS47L15_FLL1_CONTROL_1 - 1;
        break;
    case CS47L15_FLLAO:
        fll->base = CS47L15_FLL_AO_CONTROL_1 - 1;
        break;
    default:
        ret = CS47L15_STATUS_FAIL;
        break;
    }

    return ret;
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

        ret = cs47l15_fll_init(driver, CS47L15_FLL1);
        if(ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }

        ret = cs47l15_fll_init(driver, CS47L15_FLLAO);
        if(ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }

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
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

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
    ret = regmap_write_array(cp, (uint32_t *) driver->config.syscfg_regs, driver->config.syscfg_regs_total);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }

    // Unmask interrupts
    // Omit first mask register, as BOOT_DONE_EINT1 is enabled by default
    for (uint32_t i = 1; i < (sizeof(cs47l15_event_data) / sizeof(irq_reg_t)); i++)
    {
        ret = cs47l15_update_reg(driver,
                                 CS47L15_IRQ1_MASK_1 + cs47l15_event_data[i].irq_reg_offset,
                                 cs47l15_event_data[i].mask,
                                 0);
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
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (addr == 0 || data == NULL || size == 0 || size % 2 != 0)
    {
        return CS47L15_STATUS_FAIL;
    }

    ret = regmap_write_block(cp, addr, data, size);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }

    return CS47L15_STATUS_OK;
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

uint32_t cs47l15_fll_config(cs47l15_t *driver,
                            uint32_t fll_clk_id,
                            uint32_t src,
                            uint32_t freq_in,
                            uint32_t freq_out)
{
    uint32_t ret = CS47L15_STATUS_OK;

    switch (fll_clk_id)
    {
    case CS47L15_FLL1_REFCLK:
        return cs47l15_set_fll_refclk(driver, &driver->fll[CS47L15_FLL1],
                                      src, freq_in, freq_out);
    case CS47L15_FLL1_SYNCCLK:
        return cs47l15_set_fll_syncclk(driver, &driver->fll[CS47L15_FLL1],
                                       src, freq_in);
    case CS47L15_FLLAO_REFCLK:
        return cs47l15_set_fll_ao_refclk(driver, &driver->fll[CS47L15_FLLAO],
                                         src, freq_in, freq_out);
    default:
        return CS47L15_STATUS_FAIL;
    }
    return ret;
}

uint32_t cs47l15_fll_enable(cs47l15_t *driver, uint32_t fll_id)
{
    uint32_t ret = CS47L15_STATUS_OK;
    cs47l15_fll_t *fll;
    bool enabled;
    uint32_t sync_ena_bit = 0x0;

    fll = &driver->fll[fll_id];

    // If already enabled, return OK
    ret = cs47l15_is_enabled_fll(driver, fll->base, &enabled);
    if (ret != CS47L15_STATUS_OK)
    {
        return ret;
    }
    if (enabled)
    {
        return CS47L15_STATUS_OK;
    }

    switch (fll_id)
    {
    case CS47L15_FLL1:
        // Set into freerun
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                                 CS47L15_FLL1_FREERUN_MASK,
                                 CS47L15_FLL1_FREERUN);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }

        // Enable syncclk if requested
        if (driver->fll[CS47L15_FLL1].sync_src >= 0)
        {
            sync_ena_bit = CS47L15_FLL1_SYNC_ENA;
        }
        ret = cs47l15_write_reg(driver,
                                CS47L15_FLL1_SYNCHRONISER_1,
                                sync_ena_bit);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }

        // Enable refclk
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                                 CS47L15_FLL1_ENA_MASK,
                                 CS47L15_FLL1_ENA);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }

        // Clear freerun
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLL_CONTROL_1_OFFS,
                                 CS47L15_FLL1_FREERUN_MASK,
                                 0);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }
        break;

    case CS47L15_FLLAO:
        /* Set hold */
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                                CS47L15_FLL_AO_HOLD_MASK, CS47L15_FLL_AO_HOLD);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }

        // Enable AO
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                                 CS47L15_FLL_AO_ENA_MASK, CS47L15_FLL_AO_ENA);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }

        /* Release the hold so that fll_ao locks to external frequency */
        ret = cs47l15_update_reg(driver,
                                 fll->base + CS47L15_FLLAO_CONTROL_1_OFFS,
                                 CS47L15_FLL_AO_HOLD, 0);
        if (ret != CS47L15_STATUS_OK)
        {
            return ret;
        }
        break;

    default:
        ret = CS47L15_STATUS_FAIL;
        break;
    }

    return ret;
}

/**
 * Disable an FLL
 *
 */
uint32_t cs47l15_fll_disable(cs47l15_t *driver, uint32_t fll_id)
{
    uint32_t ret = CS47L15_STATUS_OK;

    switch (fll_id)
    {
    case CS47L15_FLL1:
        // Disable both syncclk and refclk
        cs47l15_disable_fll(driver, driver->fll);
        break;
    case CS47L15_FLLAO:
        cs47l15_disable_fll_ao(driver, driver->fll);
        break;

    default:
        ret = CS47L15_STATUS_FAIL;
        break;
    }
    return ret;
}

/**
 * Wait for short time for an FLL to achieve lock
 *
 */
uint32_t cs47l15_fll_wait_for_lock(cs47l15_t *driver, uint32_t fll_id)
{
    uint32_t i, mask, temp_reg_val, ret=CS47L15_STATUS_OK;

    switch(fll_id)
    {
        case CS47L15_FLL1:
            mask = CS47L15_FLL1_LOCK_STS1_MASK;
            break;
        case CS47L15_FLLAO:
            mask = CS47L15_FLL_AO_LOCK_STS1_MASK;
            break;
        default:
            return CS47L15_STATUS_FAIL;
            break;
    }

    for (i = 0; i < 30; i++)
    {
        ret = cs47l15_read_reg(driver, CS47L15_IRQ1_RAW_STATUS_2, &temp_reg_val);
        if (ret == CS47L15_STATUS_FAIL)
        {
            return ret;
        }
        if (temp_reg_val & mask)
        {
            return CS47L15_STATUS_OK;
        }
        bsp_driver_if_g->set_timer(10, NULL, NULL);
    }
    return CS47L15_STATUS_FAIL;
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
