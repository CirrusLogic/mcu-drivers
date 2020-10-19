/**
 * @file cs40l25_ext.c
 *
 * @brief The CS40L25 Driver Extended API module
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
#include "cs40l25_ext.h"
#include "bsp_driver_if.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
#define CS40L25_DYNAMIC_F0_TABLE_SIZE           (20)
#define CS40L25_POLL_DYNAMIC_REDC_TOTAL         (30)
#endif //CS40L25_ALGORITHM_DYNAMIC_F0

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Get the HALO HEARTBEAT
 *
 */
uint32_t cs40l25_get_halo_heartbeat(cs40l25_t *driver, uint32_t *hb)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    if (hb == NULL)
    {
        return CS40L25_STATUS_FAIL;
    }

    if (driver->state == CS40L25_STATE_POWER_UP)
    {
        req.id = CS40L25_CONTROL_ID_GET_REG;
        fa.address = DSP_BHM_HALO_HEARTBEAT_REG;
    }
    else if (driver->state == CS40L25_STATE_CAL_POWER_UP)
    {
        req.id = CS40L25_CONTROL_ID_GET_SYM;
        fa.id = CS40L25_SYM_FIRMWARE_HALO_HEARTBEAT;
    }
    else if (driver->state == CS40L25_STATE_DSP_POWER_UP)
    {
        req.id = CS40L25_CONTROL_ID_GET_SYM;
        fa.id = CS40L25_CAL_SYM_FIRMWARE_HALO_HEARTBEAT;
    }

    req.arg = (void *) &fa;
    fa.value = (uint32_t) hb;
    fa.size = 32;

    ret = cs40l25_control(driver, req);

    return ret;
}

/**
 * Update the HALO FW Haptic Configuration
 *
 */
uint32_t cs40l25_update_haptic_config(cs40l25_t *driver, cs40l25_haptic_config_t *config)
{
    uint32_t ret;
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};

    if (config == NULL)
    {
        return CS40L25_STATUS_FAIL;
    }

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;

    fa.id = CS40L25_SYM_FIRMWARE_GPIO_ENABLE;
    fa.value = 0;
    fa.size = 32;

    ret = cs40l25_control(driver, req);
    if (ret)
    {
        return ret;
    }

    req.arg = (void *) &fa;

    // Set CTRL_PORT_GAIN_CONTROL and GPI_GAIN_CONTROL
    fa.id = CS40L25_SYM_FIRMWARE_GAIN_CONTROL;
    fa.value = (uint32_t) config->cp_gain_control;
    fa.size = 10;
    fa.shift = 4;

    ret = cs40l25_control(driver, req);
    if (ret)
    {
        return ret;
    }

    req.arg = (void *) &fa;

    fa.value = (uint32_t) config->gpio_gain_control;
    fa.shift = 14;

    ret = cs40l25_control(driver, req);
    if (ret)
    {
        return ret;
    }

    req.arg = (void *) &fa;

    for (uint8_t i = 0; i < 4; i++)
    {
        fa.id = CS40L25_SYM_FIRMWARE_GPIO_BUTTONDETECT;
        fa.value = (uint32_t) config->gpio_trigger_config[i].enable;
        fa.size = 1;
        fa.shift = i;
        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        req.arg = (void *) &fa;

        fa.id = CS40L25_SYM_FIRMWARE_INDEXBUTTONPRESS;
        fa.value = (uint32_t) config->gpio_trigger_config[i].button_press_index;
        fa.address = (i * 4);
        fa.size = 32;
        fa.shift = 0;
        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        req.arg = (void *) &fa;

        fa.id = CS40L25_SYM_FIRMWARE_INDEXBUTTONRELEASE;
        fa.value = (uint32_t) config->gpio_trigger_config[i].button_release_index;
        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        req.arg = (void *) &fa;
    }

    fa.id = CS40L25_SYM_FIRMWARE_GPIO_ENABLE;
    fa.value = (uint32_t) config->gpio_enable;
    fa.size = 32;
    ret = cs40l25_control(driver, req);

    return ret;
}

/**
 * Trigger the ROM Mode (BHM) Haptic Effect
 *
 */
uint32_t cs40l25_trigger_bhm(cs40l25_t *driver)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    req.id = CS40L25_CONTROL_ID_SET_REG;
    req.arg = (void *) &fa;
    fa.address = DSP_BHM_BUZZ_TRIGGER_REG;
    fa.value = (uint32_t) 0x1;
    fa.ack_ctrl = true;
    fa.ack_reset = 0x0;
    fa.size = 32;
    fa.shift = 0;

    ret = cs40l25_control(driver, req);

    return ret;
}

#ifdef CS40L25_ALGORITHM_VIBEGEN
/**
 * Trigger RAM Mode Haptic Effects
 *
 */
uint32_t cs40l25_trigger(cs40l25_t *driver, uint32_t index, uint32_t duration_ms)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    if (duration_ms == 0)
    {
        req.id = CS40L25_CONTROL_ID_SET_REG;
        req.arg = (void *) &fa;

        fa.address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG;
    }
    else
    {
        req.id = CS40L25_CONTROL_ID_SET_SYM;
        req.arg = (void *) &fa;
        fa.id = CS40L25_SYM_VIBEGEN_TIMEOUT_MS;
        fa.value = (uint32_t) duration_ms;
        fa.size = 32;

        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        req.id = CS40L25_CONTROL_ID_SET_REG;
        req.arg = (void *) &fa;

        fa.address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_2_REG;
        fa.id = 0;
    }

    fa.value = (uint32_t) index;
    fa.ack_ctrl = true;
    fa.ack_reset = 0xFFFFFFFF;
    fa.size = 32;
    fa.shift = 0;

    ret = cs40l25_control(driver, req);

    return ret;
}

/**
 * Enable the HALO FW Click Compensation
 *
 */
uint32_t cs40l25_set_click_compensation_enable(cs40l25_t *driver, bool enable)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_VIBEGEN_COMPENSATION_ENABLE;
    fa.value = (uint32_t) enable;
    fa.size = 32;

    ret = cs40l25_control(driver, req);

    return ret;
}
#endif //CS40L25_ALGORITHM_VIBEGEN

#ifdef CS40L25_ALGORITHM_CLAB
/**
 * Enable the HALO FW CLAB Algorithm
 *
 */
uint32_t cs40l25_set_clab_enable(cs40l25_t *driver, bool enable)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_CLAB_CLAB_ENABLED;
    fa.value = (uint32_t) enable;
    fa.size = 32;

    ret = cs40l25_control(driver, req);

    return ret;
}

/**
 * Set the CLAB Peak Amplitude Control
 *
 */
uint32_t cs40l25_set_clab_peak_amplitude(cs40l25_t *driver, uint32_t amplitude)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_CLAB_PEAK_AMPLITUDE_CONTROL;
    fa.value = amplitude;
    fa.size = 32;

    return cs40l25_control(driver, req);
}

#endif //CS40L25_ALGORITHM_CLAB

#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
/**
 * Enable the HALO FW Dynamic F0 Algorithm
 *
 */
uint32_t cs40l25_set_dynamic_f0_enable(cs40l25_t *driver, bool enable)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_DYNAMIC_F0_DYNAMIC_F0_ENABLED;
    fa.value = (uint32_t) enable;
    fa.size = 32;

    ret = cs40l25_control(driver, req);

    return ret;
}

/**
 * Get the Dynamic F0
 *
 */
uint32_t cs40l25_get_dynamic_f0(cs40l25_t *driver, cs40l25_dynamic_f0_table_entry_t *f0_entry)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    cs40l25_dynamic_f0_table_entry_t f0_read;
    uint32_t ret;

    if (f0_entry->index >= CS40L25_DYNAMIC_F0_TABLE_SIZE)
    {
        return CS40L25_STATUS_FAIL;
    }

    req.id = CS40L25_CONTROL_ID_GET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_DYNAMIC_F0_DYN_F0_TABLE;
    fa.value = (uint32_t) &(f0_read.word);
    fa.size = 32;

    uint8_t i;
    for (i = 0; i < CS40L25_DYNAMIC_F0_TABLE_SIZE; i++)
    {
        fa.address = i;

        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        if (f0_entry->index == f0_read.index)
        {
            f0_entry->f0 = f0_read.f0;
            break;
        }
    }

    // Set to default of table entry to indicate index not found
    if (i >= CS40L25_DYNAMIC_F0_TABLE_SIZE)
    {
        f0_entry->word = CS40L25_DYNAMIC_F0_TABLE_ENTRY_DEFAULT;
    }

    return ret;
}

/**
 * Get the Dynamic ReDC
 *
 */
uint32_t cs40l25_get_dynamic_redc(cs40l25_t *driver, uint32_t *redc)
{
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
    uint32_t ret;

    // The driver will set the dynamic_redc control to -1 (0xFFFFFF)
    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;
    fa.id = CS40L25_SYM_DYNAMIC_F0_DYNAMIC_REDC;
    fa.value = 0xFFFFFF;
    fa.size = 32;

    ret = cs40l25_control(driver, req);
    if (ret)
    {
        return ret;
    }

    // The PowerControl (MBOX_4) register must be set to WAKEUP (2)
    req.id = CS40L25_CONTROL_ID_SET_REG;
    fa.address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG;
    fa.value = (uint32_t) CS40L25_POWERCONTROL_WAKEUP;
    fa.ack_ctrl = true;
    fa.ack_reset = CS40L25_POWERCONTROL_NONE;

    ret = cs40l25_control(driver, req);
    if (ret)
    {
        return ret;
    }

    // Set up for polling DYNAMIC_REDC in the loop
    req.id = CS40L25_CONTROL_ID_GET_SYM;
    fa.id = CS40L25_SYM_DYNAMIC_F0_DYNAMIC_REDC;
    fa.value = (uint32_t) redc;
    fa.ack_ctrl = false;

    uint8_t i;
    for (i = 0; i < CS40L25_POLL_DYNAMIC_REDC_TOTAL; i++)
    {
        // Wait 10ms before reading again
        ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                         NULL,
                                         NULL);
        if (ret)
        {
            return CS40L25_STATUS_FAIL;
        }

        // The dynamic_redc register contents will remain at -1 until the calculation is complete
        ret = cs40l25_control(driver, req);
        if (ret)
        {
            return ret;
        }

        if (*redc != 0xFFFFFF)
        {
            break;
        }
    }

    if (i >= CS40L25_POLL_DYNAMIC_REDC_TOTAL)
    {
        return CS40L25_STATUS_FAIL;
    }

    return CS40L25_STATUS_OK;
}
#endif //CS40L25_ALGORITHM_DYNAMIC_F0
