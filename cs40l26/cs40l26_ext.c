/**
 * @file cs40l25_ext.c
 *
 * @brief The CS40L25 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023 All Rights Reserved, http://www.cirrus.com/
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
#include <stdio.h>
#include "cs40l26_ext.h"
#include "bsp_driver_if.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/**
 * Total entries in Dynamic F0 table
 */
#define CS40L26_DYNAMIC_F0_TABLE_SIZE           (20)

#define CS40L26_CLICK_COMPENSATION_F0_EN        (0x1)
#define CS40L26_CLICK_COMPENSATION_REDC_EN      (0x2)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
#ifdef PWLE_API_ENABLE
cs40l26_pwle_t pwle_default =
{
    .word1.wf_length = WF_LENGTH_DEFAULT,
    .word2.pwls_ms4 = PWLS_MS4,
    .word2.wait_time = WAIT_TIME_DEFAULT,
    .word2.repeat = REPEAT_DEFAULT,
    .word3.level_ms4 = LEVEL_MS4,
    .word3.time = TIME_DEFAULT,
    .word3.pwls_ls4 = PWLS_LS4,
    .word4.ext_freq = EXT_FREQ_DEFAULT,
    .word4.amp_reg = AMP_REG_DEFAULT,
    .word4.braking = BRAKING_DEFAULT,
    .word4.chirp = CHIRP_DEFAULT,
    .word4.freq = FREQ_DEFAULT,
    .word4.level_ls8 = LEVEL_LS8,
    .word5.level_ms4 = 0,
    .word5.time = TIME_DEFAULT,
    .word6.level_ls8 = 0,
    .word6.freq = FREQ_DEFAULT,
    .word6.ext_freq = EXT_FREQ_DEFAULT,
    .word6.amp_reg = AMP_REG_DEFAULT,
    .word6.braking = BRAKING_DEFAULT,
    .word6.chirp = CHIRP_DEFAULT

};

cs40l26_pwle_short_section_t pwle_short_default =
{
    .word1.time = TIME_DEFAULT,
    .word1.level_ms8 = LEVEL_MS8_DEFAULT,
    .word2.level_ls4 = LEVEL_LS4_DEFAULT,
    .word2.freq = FREQ_DEFAULT,
    .word2.chirp = CHIRP_DEFAULT,
    .word2.braking = BRAKING_DEFAULT,
    .word2.amp_reg = AMP_REG_DEFAULT,
    .word2.ext_freq = EXT_FREQ_DEFAULT
};
#endif
/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

uint32_t cs40l26_mailbox_queue_handler(cs40l26_t *driver)
{
    uint32_t val;
    uint32_t rd;
    uint32_t wt;
    uint32_t count = 0;
    uint32_t rd_sym, wt_sym;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->is_cal_boot)
    {
        rd_sym = CS40L26_CAL_SYM_MAILBOX_QUEUE_RD;
        wt_sym = CS40L26_CAL_SYM_MAILBOX_QUEUE_WT;
    }
    else
    {
        rd_sym = CS40L26_SYM_MAILBOX_QUEUE_RD;
        wt_sym = CS40L26_SYM_MAILBOX_QUEUE_WT;
    }
    do
    {
        uint32_t ret;
        if (count > CS40L26_MAILBOX_QUEUE_MAX_LEN)
        {
            return CS40L26_STATUS_FAIL;
        }

        ret = regmap_read_fw_control(cp, driver->fw_info, rd_sym, &rd);
        if (ret)
        {
            return ret;
        }
        ret = regmap_read(cp, rd, &val);
        if (ret)
        {
            return ret;
        }

        driver->mailbox_queue[count] = val;

        if ((rd + 4) <= CS40L26_DSP_MBOX_8)
        {
            rd += 4;
        }
        else
        {
            rd = CS40L26_DSP_MBOX_2;
        }
        ret = regmap_write_fw_control(cp, driver->fw_info, rd_sym, rd);
        if (ret)
        {
            return ret;
        }

        ret = regmap_read_fw_control(cp, driver->fw_info, wt_sym, &wt);
        if (ret)
        {
            return ret;
        }

        count++;
        // rd == wt: all messages read
    } while (rd != wt);

    return CS40L26_STATUS_OK;
}

/**
 * Enable the HALO FW Click Compensation
 *
 */
uint32_t cs40l26_set_click_compensation_enable(cs40l26_t *driver, bool f0_enable, bool redc_enable)
{
    uint32_t ret;
    uint32_t enable = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->fw_info == NULL || driver->fw_info->header.fw_version == 0x12345)
    {
        return CS40L26_STATUS_FAIL;
    }
    if (f0_enable)
    {
        enable |= CS40L26_CLICK_COMPENSATION_F0_EN;
    }

    if (redc_enable)
    {
        enable |= CS40L26_CLICK_COMPENSATION_REDC_EN;
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS40L26_SYM_VIBEGEN_COMPENSATION_ENABLE, enable);

    return ret;
}

/**
 * Enable the HALO FW Dynamic F0 Algorithm
 *
 */
uint32_t cs40l26_set_dynamic_f0_enable(cs40l26_t *driver, bool enable)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->fw_info == NULL || driver->fw_info->header.fw_version == 0x12345)
    {
        ret = regmap_write(cp, CS40L26_DYNAMIC_F0_ENABLED, enable);
        if (ret)
        {
            return ret;
        }

        ret = regmap_write(cp, CS40L26_DYNAMIC_F0_IMONRINGPPTHRESHOLD, 0x20C5);
        if (ret)
        {
            return ret;
        }

        ret = regmap_write(cp, CS40L26_DYNAMIC_F0_FRME_SKIP, 0x30);
        if (ret)
        {
            return ret;
        }

        ret = regmap_write(cp, CS40L26_DYNAMIC_F0_NUM_PEAKS_TOFIND, 5);
    }
    else
    {
        ret = regmap_write_fw_control(cp, driver->fw_info,
                                      CS40L26_SYM_DYNAMIC_F0_DYNAMIC_F0_ENABLED, enable);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write_fw_control(cp, driver->fw_info,
                                      CS40L26_SYM_DYNAMIC_F0_IMONRINGPPTHRESHOLD, 0x20C5);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write_fw_control(cp, driver->fw_info,
                                      CS40L26_SYM_DYNAMIC_F0_FRME_SKIP, 0x30);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write_fw_control(cp, driver->fw_info,
                                      CS40L26_SYM_DYNAMIC_F0_NUM_PEAKS_TOFIND, 5);
    }
    return ret;
}

/**
 * Get the Dynamic F0
 *
 */
uint32_t cs40l26_get_dynamic_f0(cs40l26_t *driver, cs40l26_dynamic_f0_table_entry_t *f0_entry)
{
    uint32_t ret, reg_addr;
    cs40l26_dynamic_f0_table_entry_t f0_read;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    uint8_t i;
    reg_addr = CS40L26_DYNAMIC_F0_TABLE;
    for (i = 0; i < CS40L26_DYNAMIC_F0_TABLE_SIZE; i++)
    {
        ret = regmap_read(cp, reg_addr, &(f0_read.word));
        if (ret)
        {
            return ret;
        }

        if (f0_entry->index == f0_read.index)
        {
            f0_entry->f0 = f0_read.f0;
            break;
        }

        reg_addr += 4;
    }

    // Set to default of table entry to indicate index not found
    if (i >= CS40L26_DYNAMIC_F0_TABLE_SIZE)
    {
        f0_entry->word = CS40L26_DYNAMIC_F0_TABLE_ENTRY_DEFAULT;
    }

    return ret;
}

#ifdef PWLE_API_ENABLE
uint32_t cs40l26_trigger_pwle(cs40l26_t *driver, rth_pwle_section_t **s)
{
    int i;
    uint32_t ret, addr;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    addr = CS40L26_OWT_SLOT0_DATA;
    ret = regmap_write(cp, CS40L26_OWT_SLOT0_TYPE, 12);
    if (ret)
    {
        return ret;
    }

    pwle_default.word3.pwls_ls4 = 2;
    pwle_default.word3.time = s[0]->duration;
    pwle_default.word4.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word3.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word4.freq = s[0]->freq;
    pwle_default.word6.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word5.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word5.time = s[1]->duration;
    pwle_default.word6.freq = s[1]->freq;

    for (i = 0; i < 6; i++)
    {
        ret = regmap_write(cp, addr, pwle_default.words[i]);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
    }
    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_TRIGGER_RTH);

    return ret;
}

uint32_t cs40l26_trigger_pwle_advanced(cs40l26_t *driver, rth_pwle_section_t **s, uint8_t repeat, uint8_t num_sections)
{
    uint32_t ret, addr;
    int i;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    addr = CS40L26_OWT_SLOT0_DATA;
    ret = regmap_write(cp, CS40L26_OWT_SLOT0_TYPE, 12);
    if (ret)
    {
        return ret;
    }

    pwle_default.word2.repeat = repeat;
    pwle_default.word2.pwls_ms4 = (num_sections & 0xF0) >> 4;
    pwle_default.word3.pwls_ls4 = (num_sections & 0xF);
    pwle_default.word3.time = s[0]->duration;
    pwle_default.word4.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word3.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word4.freq = s[0]->freq;
    pwle_default.word4.amp_reg = (s[0]->half_cycles ? 1 : 0);
    pwle_default.word4.chirp = (s[0]->chirp ? 1 : 0);
    pwle_default.word6.level_ls8 = s[1]->level & 0xFF;
    pwle_default.word5.level_ms4 = (s[1]->level & 0xF00) >> 8;
    pwle_default.word5.time = s[1]->duration;
    pwle_default.word6.freq = s[1]->freq;
    pwle_default.word6.amp_reg = (s[1]->half_cycles ? 1 : 0);
    pwle_default.word6.chirp = (s[1]->chirp ? 1 : 0);

    for (i = 0; i < 6; i++)
    {
        ret = regmap_write(cp, addr, pwle_default.words[i]);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
    }
    for (i = 2; i < num_sections; i++)
    {
        pwle_short_default.word1.time = s[i]->duration;
        pwle_short_default.word1.level_ms8 = (s[i]->level & 0xFF0) >> 4;
        pwle_short_default.word2.level_ls4 = s[i]->level & 0x00F;
        pwle_short_default.word2.freq = s[i]->freq;
        pwle_short_default.word2.amp_reg = (s[i]->half_cycles ? 1 : 0);
        pwle_short_default.word2.chirp = (s[i]->chirp ? 1 : 0);

        ret = regmap_write(cp, addr, pwle_short_default.words[0] >> 4);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
        uint32_t data = (pwle_short_default.words[0]&0xF) << 20;
        data |= (pwle_short_default.words[1]) >> 4;
        ret = regmap_write(cp, addr, data);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
        ret = regmap_write(cp, addr, (pwle_short_default.words[1]&0xF) << 20);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_TRIGGER_RTH);

    return ret;
}
#endif

uint32_t cs40l26_pack_pcm_data(regmap_cp_config_t *cp, int index, uint32_t *word, uint8_t data, uint32_t *addr)
{
    uint32_t ret;

    switch(index%3)
    {
    case 0:
        *word = *word | (data << 16);
        break;
    case 1:
        *word = *word | (data << 8);
        break;
    case 2:
        *word = *word | (data);
        ret = regmap_write(cp, *addr, *word);
        if (ret)
        {
            return ret;
        }
        *addr += 0x4;
        *word = 0;
        break;
    default:
        break;
    };

    return CS40L26_STATUS_FAIL;
}

uint32_t cs40l26_trigger_pcm(cs40l26_t *driver, uint8_t *s, uint32_t num_sections, uint16_t buffer_size_samples, uint16_t f0, uint16_t redc)
{
    uint32_t ret, addr;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L26_OWT_SLOT0_TYPE, CS40L26_RTH_TYPE_PCM); //write the type of waveform
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS40L26_OWT_SLOT0_OFFSET, 3); //set when the data starts
    if (ret)
    {
        return ret;
    }

    addr = CS40L26_OWT_SLOT0_DATA;
    ret = regmap_write(cp, addr, num_sections); //Writes the wavelengh that also is the number of sections
    if (ret)
    {
        return ret;
    }
    addr += 0x4;
    ret = regmap_write(cp, addr, (f0 << 12) | redc); //Writes F0 and ReDC Values
    if (ret)
    {
        return ret;
    }
    addr += 0x4;
    uint32_t word = 0;
    for (int i = 0; i < buffer_size_samples; i++)
    {
        ret = cs40l26_pack_pcm_data(cp, i, &word, s[i], &addr);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_TRIGGER_RTH);
    if (ret)
    {
        return ret;
    }
    if (buffer_size_samples < num_sections)
    {
        for (int i = buffer_size_samples; i < num_sections; i++)
        {
            ret = cs40l26_pack_pcm_data(cp, i, &word, s[i], &addr);
            if (ret)
            {
                return ret;
            }
        }
        if ((num_sections % 3) != 0)
        {
            ret = regmap_write(cp, addr, word);
            if (ret)
            {
                return ret;
            }
        }

    }
    return ret;
}

uint32_t cs40l26_gpi_pmic_mute_enable(cs40l26_t *driver, bool enable)
{
    return regmap_update_fw_control(REGMAP_GET_CP(driver),
                                    driver->fw_info,
                                    CS40L26_SYM_FW_RAM_EXT_GPI_PMIC_MUTE_ENABLE,
                                    CS40L26_GPI_PMIC_MUTE_ENABLE_MASK,
                                    enable);
}

uint32_t cs40l26_gpi_pmic_mute_configure(cs40l26_t *driver, uint8_t gpi, bool level)
{
    return regmap_update_fw_control(REGMAP_GET_CP(driver),
                                    driver->fw_info,
                                    CS40L26_SYM_FW_RAM_EXT_GPI_PMIC_MUTE_ENABLE,
                                    CS40L26_GPI_PMIC_MUTE_GPI_LEVEL_MASK,
                                    (gpi << CS40L26_GPI_PMIC_MUTE_GPI_SHIFT) | (level << CS40L26_GPI_PMIC_MUTE_LEVEL_SHIFT));
}

uint32_t cs40l26_owt_upload_effect(cs40l26_t *driver, uint32_t *effect, uint8_t size)
{
    uint32_t ret, addr, offset, data;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read_fw_control(cp, driver->fw_info, CS40L26_SYM_VIBEGEN_OWT_SIZE_XM, &data);
    if ((size > data) || ret)
    {
        return ret;
    }

    ret = regmap_read_fw_control(cp, driver->fw_info, CS40L26_SYM_VIBEGEN_OWT_NEXT_XM, &offset);
    if(ret)
    {
        return ret;
    }

    addr = CS40l26_VIBEGEN_OWT_WAVETABLE + (4*offset);

    for (int i = 0; i < size; i++)
    {
        ret = regmap_write(cp, addr, effect[i]);
        if (ret)
        {
            return ret;
        }
        addr+=0x4;
    }

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_CMD_OWT_PUSH);

    return ret;
}

uint32_t cs40l26_owt_reset_table(cs40l26_t *driver)
{
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    return regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_CMD_OWT_RESET);
}
