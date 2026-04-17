/*
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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

#include "cs40l5x_bsp.h"
#include "cs40l5x.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <strings.h>

LOG_MODULE_REGISTER(cirrus_cs40l5x);

struct cs40l5x_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec reset_gpio;
    void (*irq_cfg_func)(void);
    void (*irq_enable_func)(void);
    void (*irq_disable_func)(void);
};

int cs40l5x_i2c_write_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                    const uint32_t value)
{
    uint8_t addr_buf[4], data_buf[4], msg_buf[8];
    int i, j = 4;

    sys_put_be32(reg_addr, addr_buf);
    sys_put_be32(value, data_buf);

    for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
        msg_buf[i] = addr_buf[i];
        msg_buf[j] = data_buf[i];
        j++;
    }

    return i2c_write_dt(spec, msg_buf, sizeof(msg_buf));
}

int cs40l5x_i2c_read_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t *value)
{
    uint8_t write_buf[4], read_buf[4];
    int ret;

    sys_put_be32(reg_addr, write_buf);

    ret = i2c_write_read_dt(spec, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf));
    if (ret < 0) {
        LOG_INF("cs40l5x_i2c_read_reg_dt error\n");
        return ret;
    }

    *value = sys_get_be32(read_buf);

    return 0;
}

int cs40l5x_update_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                 const uint32_t mask, const uint32_t value)
{
    uint32_t old_value, new_value;
    int ret;

    ret = cs40l5x_i2c_read_reg_dt(spec, reg_addr, &old_value);
    if (ret < 0) {
        return ret;
    }

    new_value = (old_value & ~mask) | (value & mask);
    if (new_value == old_value) {
        return 0;
    }

    return cs40l5x_i2c_write_reg_dt(spec, reg_addr, new_value);
}

/* write addr/word pairs */
int cs40l5x_write_array_dt(const struct i2c_dt_spec *spec,
                    const uint32_t *array, uint32_t words)
{
    int i, ret;

    for (i = 0; i < words; i+=2) {
        ret = cs40l5x_i2c_write_reg_dt(spec, array[i], array[i+1]);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

/**
 * Reads a register for a specific value for a specified amount of tries while waiting between reads.
 *
 */
int cs40l5x_poll_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t value, uint32_t tries, uint32_t delay)
{
    uint32_t tmp;
    int i, ret;

    for (i = 0; i < tries; i++) {
        ret = cs40l5x_i2c_read_reg_dt(spec, reg_addr, &tmp);
        if (ret < 0) {
            return ret;
        }

        if (tmp == value)
          return 0;

        k_msleep(delay);
    }
    return 0;
}

int cs40l5x_write_acked_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                                uint32_t val, uint32_t acked_val,  uint8_t tries,  uint32_t delay)
{
    int i, ret;

    ret = cs40l5x_i2c_write_reg_dt(spec, reg_addr, val);
    if (ret < 0) {
        return ret;
    }

    for (i = 0 ; i < tries; i++) {
        uint32_t temp_reg_val;

        k_msleep(delay);

        cs40l5x_i2c_read_reg_dt(spec, reg_addr, &temp_reg_val);

        if (temp_reg_val == acked_val)
        {
            return 0;
        }
    }

    return -1;
}

int cs40l5x_i2c_write_bulk_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    const uint8_t *buf, unsigned int num_bytes, uint16_t bus_addr)
{
    uint8_t addr_buf[4], *msg_buf;
    int i = 4, ret = 0;

    LOG_INF("cs40l5x_i2c_write_bulk_bus: reg=%x size=%d\n", reg_addr, num_bytes);

    msg_buf = k_malloc(num_bytes+4);
    if (!msg_buf)
        return -ENOMEM;

    sys_put_be32(reg_addr, addr_buf);

    for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
        msg_buf[i] = addr_buf[i];
    }

    bytecpy(msg_buf+4, buf, num_bytes);

    ret = i2c_write(spec->bus, msg_buf, 4 + num_bytes, bus_addr);

    k_free(msg_buf);
    return ret;
}

int cs40l5x_i2c_write_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    uint32_t val, uint16_t bus_addr)
{
    uint8_t addr_buf[4], data_buf[4], *msg_buf;
    int ret = 0;

    LOG_INF("cs40l5x_i2c_write_bus: reg=%x\n", reg_addr);

    msg_buf = k_malloc(8);
    if (!msg_buf)
        return -ENOMEM;

    sys_put_be32(reg_addr, addr_buf);
    sys_put_be32(val, data_buf);

    bytecpy(msg_buf, addr_buf, 4);
    bytecpy(msg_buf+4, data_buf, 4);

    ret = i2c_write(spec->bus, msg_buf, 8, bus_addr);

    k_free(msg_buf);
    return ret;
}

int cs40l5x_i2c_write_bulk_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    const uint8_t *buf, unsigned int num_bytes)
{
    uint8_t addr_buf[4], *msg_buf;
    int i = 4, ret = 0;

    LOG_INF("cs40l5x_i2c_write_bulk_dt: reg=%x size=%d\n", reg_addr, num_bytes);

    msg_buf = k_malloc(num_bytes+4);
    if (!msg_buf)
        return -ENOMEM;

    sys_put_be32(reg_addr, addr_buf);

    for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
        msg_buf[i] = addr_buf[i];
    }

    bytecpy(msg_buf+4, buf, num_bytes);

    ret = i2c_write_dt(spec, msg_buf, sizeof(msg_buf) + num_bytes);

    k_free(msg_buf);
    return ret;
}

int cs40l5x_set_haptic_cfg(const struct device *dev, struct cs40l5x_haptic_source_config* hap_cfg)
{
    struct cs40l5x_bsp* data = dev->data;
    data->hap_cfg = *hap_cfg;
    return 0;
}

static int cs40l5x_write_fw_blocks(struct i2c_dt_spec *i2c, halo_boot_block_t *blocks, int num_blocks)
{
    int i, ret;
    halo_boot_block_t block;
    uint32_t bytes, address;
    uint8_t* buffer;

    for (i = 0; i < num_blocks; i++) {
        block = blocks[i];
        bytes = block.block_size;
        address = block.address;
        buffer = (uint8_t*)block.bytes;
        ret = cs40l5x_i2c_write_bulk_dt(i2c, address, buffer, bytes);
        if (ret != 0)
            return ret;
        k_msleep(5);
    }

    return 0;
}

static int cs40l5x_gpi_get_level(cs40l5x_t *drv, unsigned int gpio)
{
    uint32_t gpio_status;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    if (gpio > 13)
        return -1;

    regmap_read(i2c, CS40L5X_GPIO_STATUS1, &gpio_status);

    return ((gpio_status & (1 << (gpio - 1))) != 0);
}

enum cs40l5x_tuning_set {
    CS40L5X_TUNING_SET_A,
    CS40L5X_TUNING_SET_B,
};

static unsigned int get_tuning_set(cs40l5x_t *drv)
{
    //struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    int gpi_level;

    gpi_level = cs40l5x_gpi_get_level(drv, 1);
    return gpi_level;
}

static int cs40l5x_firmware_load(cs40l5x_t *drv)
{
    int num_blocks, ret;
    halo_boot_block_t *blocks;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    uint32_t tuning_set;

    tuning_set = get_tuning_set(drv);

    num_blocks = cs40l5x_total_fw_blocks;
    blocks = (halo_boot_block_t*)cs40l5x_fw_blocks;
    ret = cs40l5x_write_fw_blocks(i2c, blocks, num_blocks);
    if (ret != 0)
        return ret;

    num_blocks = cs40l5x_total_coeff_blocks_0;
    blocks = (halo_boot_block_t*)cs40l5x_coeff_0_blocks;
    ret = cs40l5x_write_fw_blocks(i2c, blocks, num_blocks);
    if (ret != 0)
        return ret;

    num_blocks = cs40l5x_total_coeff_blocks_1;
    blocks = (halo_boot_block_t*)cs40l5x_coeff_1_blocks;
    ret = cs40l5x_write_fw_blocks(i2c, blocks, num_blocks);
    if (ret != 0)
        return ret;

    LOG_INF("Loaded wavetable");
    return 0;
}

static int cs40l5x_clear_gpio_triggers(cs40l5x_t *drv)
{
    int i;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    for (i = 0; i < 16; i++)
        regmap_write(i2c, CS40L5X_GPIO_HANDLERS_BASE + i*4, 0x1FF);

    return 0;
}

static int cs40l5x_setup_gpi(cs40l5x_t *drv, unsigned int gpio)
{
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    if (gpio > 13)
        return -1;

    regmap_write(i2c, CS40L5X_GPIO_CTRL1 + (4 * (gpio - 1)),
        CS40L5X_GPIO_CTRL_DIR_BITMASK | CS40L5X_GPIO_CTRL_FN_INPUT_OUTPUT);
    return 0;
}

static uint32_t cs40l5x_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
    k_msleep(duration_ms);
    return 0;
}

static uint32_t cs40l5x_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
    //TODO
    return 0;
}

static uint32_t cs40l5x_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
    //TODO
    return 0;
}

bsp_driver_if_t cs40l5x_bsp_driver_if = {
    .set_timer = cs40l5x_set_timer,
    .register_gpio_cb = cs40l5x_register_gpio_cb,
    .set_gpio = cs40l5x_set_gpio
};

bsp_driver_if_t *bsp_driver_if_g = &cs40l5x_bsp_driver_if;

/* ===================================================================================== */
static int cs40l5x_reset_owt(cs40l5x_t *drv)
{
    uint32_t ret;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    ret = regmap_write(i2c, CS40L5X_DSP_VIRTUAL1_MBOX_1, CS40L5X_DSP_MBOX_OWT_RESET);
    if(ret)
    {
        return ret;
    }
    return BSP_STATUS_OK;
}

static int cs40l5x_init(const struct device *dev)
{
    struct cs40l5x_config *config = (struct cs40l5x_config*)dev->config;
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t val;
    int ret;
    LOG_INF("cs40l5x_init\n");

    drv->config.bsp_config.i2c = &config->i2c;
    drv->config.syscfg_regs = cs40l5x_syscfg_regs;
    drv->config.syscfg_regs_total = CS40L5X_SYSCFG_REGS_TOTAL;
    drv->config.is_ext_bst = true;

    static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_NODELABEL(haptic1), reset_gpios);

    if (!gpio_is_ready_dt(&reset)) {
        LOG_INF("cs40l5x reset pin GPIO port is not ready.\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&reset, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_INF("cs40l5x configuring reset pin failed: %d\n", ret);
        return 0;
    }

    drv->config.bsp_config.reset_gpio_id = &reset;
    // Activate RESET for at least T_RLPW (1ms)
    gpio_pin_set_dt(&reset, BSP_GPIO_ACTIVE);
    k_msleep(2);
    // Wait for Lochnagar to boot
    gpio_pin_set_dt(&reset, BSP_GPIO_INACTIVE);
    k_msleep(2500);

    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_INF("cs40l5x no I2C\n");
        return -ENODEV;
    }

    ret = cs40l5x_reset(drv);
    if (ret < 0) {
        return ret;
    }

    regmap_read(&config->i2c, FIRMWARE_CS40L5X_HALO_STATE, &val);
    LOG_INF("cs40l5x_init: HALO_STATE = %x\n", val);

    LOG_INF("cs40l5x_calibrate\n");
    ret = cs40l5x_calibrate(drv);
    if (ret < 0) {
        LOG_INF("cs40l5x_calibrate error\n");
        return ret;
    }

    ret = cs40l5x_boot(drv, NULL);
    if (ret < 0) {
        LOG_INF("cs40l5x_boot error\n");
        return ret;
    }

    cs40l5x_setup_gpi(drv, 1);

    LOG_INF("cs40l5x_firmware_load\n");
    ret = cs40l5x_firmware_load(drv);
    if (ret < 0) {
        LOG_INF("cs40l5x_firmware_load error\n");
        return ret;
    }

    ret = regmap_write(&config->i2c, CS40L5X_DSP1_CCM_CORE_CONTROL, 0x00000281);
    if (ret < 0) {
        LOG_INF("cs40l5x_boot error\n");
        return ret;
    }

    cs40l5x_clear_gpio_triggers(drv);
    cs40l5x_reset_owt(drv);

    k_msleep(1000);

    /* to-do */
    regmap_read(&config->i2c, FIRMWARE_CS40L5X_HALO_STATE, &val);
    LOG_INF("cs40l5x_init: HALO_STATE = %x\n", val);

//    config->irq_cfg_func();
//    config->irq_enable_func();

    return 0;
}

static int haptics_cs40l5x_stop_output(const struct device *dev)
{
    return 0;
}

static int haptics_cs40l5x_start_output(const struct device *dev)
{
    struct cs40l5x_bsp *data = (struct cs40l5x_bsp*)dev->data;
    LOG_INF("haptics_cs40l5x_start_output, bank=%x, index=%x\n", data->hap_cfg.bank, data->hap_cfg.index);

    cs40l5x_trigger(&data->priv, data->hap_cfg.index, data->hap_cfg.bank);

    return 0;
}

int bsp_cs40l5x_trigger_owt(const struct device *dev, int owt_idx)
{
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t ret;

    ret = cs40l5x_trigger_owt(drv, owt_idx);
    if(ret)
    {
        LOG_ERR("Error playing out owt waveform");
        return ret;
    }
    return BSP_STATUS_OK;
}

int bsp_cs40l5x_delete_owt(const struct device *dev, int owt_idx)
{
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t ret;

    ret = cs40l5x_delete_owt(drv, owt_idx);
    if(ret)
    {
        LOG_ERR("Error deleting owt waveform");
        return ret;
    }
    return BSP_STATUS_OK;
}

int bsp_cs40l5x_write_owt_header(const struct device *dev, uint8_t num_waveforms, uint8_t repeats)
{
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t ret;

    ret = cs40l5x_write_owt_composite_header(drv, num_waveforms, repeats);
    if(ret)
    {
        LOG_ERR("Error writing owt header");
        return ret;
    }
    return BSP_STATUS_OK;
}

int bsp_cs40l5x_write_owt_section(const struct device *dev, struct cs40l5x_owt_section_params section)
{
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t ret;

    ret = cs40l5x_write_owt_composite_section(drv,
                                              section.nested_repeats,
                                              section.waveform_idx,
                                              section.amplitude,
                                              section.delay,
                                              section.owt_subwave,
                                              section.rom_subwave,
                                              section.duration_present,
                                              section.duration);
    if(ret)
    {
        LOG_ERR("Error writing owt section");
        return ret;
    }
    return BSP_STATUS_OK;
}

int bsp_cs40l5x_push_owt(const struct device *dev)
{
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;
    uint32_t ret;

    ret = cs40l5x_push_owt_composite(drv);
    if(ret)
    {
        LOG_ERR("Error pushing owt waveform");
        return ret;
    }
    return BSP_STATUS_OK;
}

int bsp_cs40l5x_write_owt_composite_one_section(const struct device *dev, struct cs40l5x_owt_section_params section)
{
    uint32_t ret;
    struct cs40l5x_bsp *data = dev->data;
    cs40l5x_t *drv = &data->priv;

    ret = cs40l5x_write_owt_composite_one_section(drv,
                                                 section.nested_repeats,
                                                 section.waveform_idx,
                                                 section.amplitude,
                                                 section.delay,
                                                 section.owt_subwave,
                                                 section.rom_subwave);
    if(ret)
    {
        LOG_ERR("Error writing owt waveform");
        return ret;
    }
    return ret;
}

int bsp_cs40l5x_get_SVC_tone_length(const struct device *dev, uint32_t* length)
{
    struct cs40l5x_config *config = (struct cs40l5x_config*)dev->config;
    uint32_t val, ret;
    uint32_t length_samples = 0;
    ret = regmap_read(&config->i2c, SVC_INIT_PH_PILOT_HI_START_SMP, &val);
    if(ret)
    {
        return ret;
    }
    length_samples += val;

    ret = regmap_read(&config->i2c, SVC_INIT_PH_OFST_CAL_START_SMP, &val);
    if(ret)
    {
        return ret;
    }
    length_samples += val;

    ret = regmap_read(&config->i2c, SVC_INIT_PH_OFST_CAL_NSMP, &val);
    if(ret)
    {
        return ret;
    }
    length_samples += val;

    ret = regmap_read(&config->i2c, SVC_INIT_PH_OFST_STL_NSMP, &val);
    if(ret)
    {
        return ret;
    }
    length_samples += val;

    //Ouput tone length in us
    *length = (length_samples * 1000) / (CS40L5X_SVC_FS / 1000);
    return BSP_STATUS_OK;
}

static uint32_t bsp_cs40l5x_convert_effect_name(char* effect_name, uint32_t *idx)
{
    for (int i = 0; i < pwleCount; i++)
    {
        if(strcasecmp(effect_name, pwleList[i]->name) == 0)
        {
            *idx = i;
            return BSP_STATUS_OK;
        }
    }
    return BSP_STATUS_FAIL;
}

static uint32_t bsp_cs40l5x_convert_msft_id(uint32_t msft_id, uint32_t *idx)
{
    for (int i = 0; i < pwleCount; i++)
    {
        if(msft_id == pwleList[i]->msft_id)
        {
            *idx = i;
            return BSP_STATUS_OK;
        }
    }
    return BSP_STATUS_FAIL;
}

int bsp_cs40l5x_host_initiated_trigger(const struct device *dev, HIH_effect effect, uint32_t intensity, uint32_t repeats, uint32_t retrigger_period, uint32_t cutoff_time)
{
    uint32_t ret, idx;
    struct cs40l5x_config *config = (struct cs40l5x_config*)dev->config;

    ret = bsp_cs40l5x_get_num_owt_wf(dev, &idx);
    if(ret)
    {
        return ret;
    }
    if(idx > 0)
    {
        ret = bsp_cs40l5x_delete_owt(dev, idx-1);
        if(ret)
        {
            return ret;
        }
    }

    //Get effect OWT index from string name or microsoft ID
    if(effect.label == EFFECT_NAME)
    {
        ret = bsp_cs40l5x_convert_effect_name(effect.HIH_effect_identifier.effectName, &idx);
        if(ret)
        {
            LOG_ERR("Error: Effect named %s not found", effect.HIH_effect_identifier.effectName);
            return ret;
        }
    }
    else if(effect.label == EFFECT_MSFT_ID)
    {
        ret = bsp_cs40l5x_convert_msft_id(effect.HIH_effect_identifier.msft_ID, &idx);
        if(ret)
        {
            LOG_ERR("Error: Effect ID 0x%x not found", effect.HIH_effect_identifier.msft_ID);
            return ret;
        }
    }
    else
    {
        idx = effect.HIH_effect_identifier.owtIdx;
    }

    struct cs40l5x_owt_section_params section =
    {
        .nested_repeats = repeats,
        .waveform_idx = idx,
        .amplitude = intensity,
        .delay = retrigger_period,
        .owt_subwave = 0,
        .rom_subwave = 0,
        .duration_present = 0,
        .duration = 0
    };

    ret = bsp_cs40l5x_write_owt_composite_one_section(dev, section);
    if(ret)
    {
        return ret;
    }

    ret = regmap_write(&config->i2c, VIBEGEN_TIMEOUT_MS, cutoff_time);
    if(ret)
    {
        return ret;
    }

    ret = bsp_cs40l5x_get_num_owt_wf(dev, &idx);
    if(ret)
    {
        return ret;
    }

    ret = bsp_cs40l5x_trigger_owt(dev, idx-1);
    if(ret)
    {
        return ret;
    }

    return BSP_STATUS_OK;
}
int bsp_cs40l5x_list_host_initiated_effects(const struct device *dev)
{
    uint32_t ret;
    printf("\n---Wavetable Waveform List---\n");
    for(int i = 1; i < pwleCount; i++)
    {
        uint32_t length_svc_us = 0;
        ret = bsp_cs40l5x_get_SVC_tone_length(dev, &length_svc_us);
        if(ret)
        {
            return ret;
        }
        uint32_t length_svc_ms = length_svc_us / 1000;
        uint32_t length_svc_ms_dec = (length_svc_us % 1000) / 10;
        uint32_t length_ms = pwleList[i]->length_us / 1000 + length_svc_ms;
        uint32_t length_ms_dec = (pwleList[i]->length_us % 1000) / 10 + length_svc_ms_dec;
        length_ms += length_ms_dec/100;
        length_ms_dec %= 100;
        if(length_ms_dec > 50) //Round to nearest ms
        {
            length_ms += 1;
        }
        printf("Name : \"%s\", Duration : %d ms\n", pwleList[i]->name, length_ms);
    }
    return BSP_STATUS_OK;
}

static const struct haptics_driver_api cs40l5x_driver_api = {
    .start_output = &haptics_cs40l5x_start_output,
    .stop_output = &haptics_cs40l5x_stop_output,
};

int bsp_cs40l5x_get_num_owt_wf(const struct device *dev, uint32_t* num)
{
    struct cs40l5x_config *config = (struct cs40l5x_config*)dev->config;
    return regmap_read(&config->i2c, VIBEGEN_OWT_NUM_OF_WAVES_XM, num);
}

#define CS40L5X_INIT(inst)                                                                   \
                                                                                             \
    static const struct cs40l5x_config cs40l5x_config_##inst = {                             \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                   \
    };                                                                                       \
                                                                                             \
    static struct cs40l5x_bsp cs40l5x_bsp_data_##inst = {                                    \
                                                                                             \
    };                                                                                       \
                                                                                             \
    PM_DEVICE_DT_INST_DEFINE(inst, cs40l5x_pm_action);                                       \
                                                                                             \
    DEVICE_DT_INST_DEFINE(inst, &cs40l5x_init, NULL, &cs40l5x_bsp_data_##inst,               \
                          &cs40l5x_config_##inst, POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY, \
                          &cs40l5x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS40L5X_INIT)

/* ===================================================================================== */

