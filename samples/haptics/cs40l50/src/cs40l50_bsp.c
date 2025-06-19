/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cs40l50_bsp.h"
#include "cs40l50.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

#include "cs40l50_firmware.h"

LOG_MODULE_REGISTER(cirrus_cs40l50);

struct cs40l50_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec reset_gpio;
    void (*irq_cfg_func)(void);
    void (*irq_enable_func)(void);
    void (*irq_disable_func)(void);
};

struct cs40l50_bsp {
    cs40l50_t priv;
    struct cs40l50_haptic_source_config hap_cfg;
};

int cs40l50_i2c_write_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
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

int cs40l50_i2c_read_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t *value)
{
    uint8_t write_buf[4], read_buf[4];
    int ret;

    sys_put_be32(reg_addr, write_buf);

    ret = i2c_write_read_dt(spec, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf));
    if (ret < 0) {
        LOG_INF("cs40l50_i2c_read_reg_dt error\n");
        return ret;
    }

    *value = sys_get_be32(read_buf);

    return 0;
}

int cs40l50_update_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                 const uint32_t mask, const uint32_t value)
{
    uint32_t old_value, new_value;
    int ret;

    ret = cs40l50_i2c_read_reg_dt(spec, reg_addr, &old_value);
    if (ret < 0) {
        return ret;
    }

    new_value = (old_value & ~mask) | (value & mask);
    if (new_value == old_value) {
        return 0;
    }

    return cs40l50_i2c_write_reg_dt(spec, reg_addr, new_value);
}

/* write addr/word pairs */
int cs40l50_write_array_dt(const struct i2c_dt_spec *spec,
                    const uint32_t *array, uint32_t words)
{
    int i, ret;

    for (i = 0; i < words; i+=2) {
        ret = cs40l50_i2c_write_reg_dt(spec, array[i], array[i+1]);
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
int cs40l50_poll_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t value, uint32_t tries, uint32_t delay)
{
    uint32_t tmp;
    int i, ret;

    for (i = 0; i < tries; i++) {
        ret = cs40l50_i2c_read_reg_dt(spec, reg_addr, &tmp);
        if (ret < 0) {
            return ret;
        }

        if (tmp == value)
          return 0;

        k_msleep(delay);
    }
    return 0;
}

int cs40l50_write_acked_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                                uint32_t val, uint32_t acked_val,  uint8_t tries,  uint32_t delay)
{
    int i, ret;

    ret = cs40l50_i2c_write_reg_dt(spec, reg_addr, val);
    if (ret < 0) {
        return ret;
    }

    for (i = 0 ; i < tries; i++) {
        uint32_t temp_reg_val;

        k_msleep(delay);

        cs40l50_i2c_read_reg_dt(spec, reg_addr, &temp_reg_val);

        if (temp_reg_val == acked_val)
        {
            return 0;
        }
    }

    return -1;
}

int cs40l50_i2c_write_bulk_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    const uint8_t *buf, unsigned int num_bytes, uint16_t bus_addr)
{
    uint8_t addr_buf[4], *msg_buf;
    int i, j = 4, ret = 0;

    LOG_INF("cs40l50_i2c_write_bulk_bus: reg=%x size=%d\n", reg_addr, num_bytes);

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

int cs40l50_i2c_write_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    uint32_t val, uint16_t bus_addr)
{
    uint8_t addr_buf[4], data_buf[4], *msg_buf;
    int i, j = 4, ret = 0;

    LOG_INF("cs40l50_i2c_write_bus: reg=%x\n", reg_addr);

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

int cs40l50_i2c_write_bulk_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
    const uint8_t *buf, unsigned int num_bytes)
{
    uint8_t addr_buf[4], *msg_buf;
    int i, j = 4, ret = 0;

    LOG_INF("cs40l50_i2c_write_bulk_dt: reg=%x size=%d\n", reg_addr, num_bytes);

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

static int cs40l50_write_fw_blocks(struct i2c_dt_spec *i2c, halo_boot_block_t *blocks, int num_blocks)
{
    int i, ret;
    halo_boot_block_t block;
    uint32_t *buffer, bytes, address;

    for (i = 0; i < num_blocks; i++) {
        block = blocks[i];
        bytes = block.block_size;
        address = block.address;
        buffer = block.bytes;
        ret = cs40l50_i2c_write_bulk_dt(i2c, address, buffer, bytes);
        if (ret != 0)
            return ret;
        k_msleep(5);
    }

    return 0;
}

static int cs40l50_gpi_get_level(cs40l50_t *drv, unsigned int gpio)
{
    uint32_t gpio_status, eint;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    if (gpio > 13)
        return -1;

    regmap_read(i2c, CS40L50_GPIO_STATUS1, &gpio_status);

    return ((gpio_status & (1 << (gpio - 1))) != 0);
}

enum cs40l50_tuning_set {
    CS40L50_TUNING_SET_A,
    CS40L50_TUNING_SET_B,
};

static unsigned int get_tuning_set(cs40l50_t *drv)
{
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    int gpi_level;

    gpi_level = cs40l50_gpi_get_level(drv, 1);
    return gpi_level;
}

static int cs40l50_firmware_load(cs40l50_t *drv)
{
    int i, num_blocks, ret;
    halo_boot_block_t *blocks;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    uint32_t tuning_set;

    tuning_set = get_tuning_set(drv);

    num_blocks = cs40l50_total_fw_blocks;
    blocks = cs40l50_fw_blocks;
    ret = cs40l50_write_fw_blocks(i2c, blocks, num_blocks);
    if (ret != 0)
        return ret;

    if (tuning_set == CS40L50_TUNING_SET_A) {
        num_blocks = cs40l50_SVC_A_total_coeff_blocks_0;
        blocks = cs40l50_SVC_A_coeff_0_blocks;
        ret = cs40l50_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret != 0)
            return ret;

        num_blocks = cs40l50_WT_A_total_coeff_blocks_2;
        blocks = cs40l50_WT_A_coeff_2_blocks;
        ret = cs40l50_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret != 0)
            return ret;

        LOG_INF("Loaded tuning set A");
    } else if (tuning_set == CS40L50_TUNING_SET_B) {
        num_blocks = cs40l50_SVC_B_total_coeff_blocks_1;
        blocks = cs40l50_SVC_A_coeff_0_blocks;
        ret = cs40l50_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret != 0)
            return ret;

        num_blocks = cs40l50_WT_B_total_coeff_blocks_3;
        blocks = cs40l50_WT_B_coeff_3_blocks;
        ret = cs40l50_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret != 0)
            return ret;

        LOG_INF("Loaded tuning set B");
    }

    return 0;
}

static int cs40l50_clear_gpio_triggers(cs40l50_t *drv)
{
    int i;
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    for (i = 0; i < 16; i++)
        regmap_write(i2c, CS40L50_GPIO_HANDLERS_BASE + i*4, 0x1FF);

    return 0;
}

static int cs40l50_setup_gpi(cs40l50_t *drv, unsigned int gpio)
{
    struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    if (gpio > 13)
        return -1;

    regmap_write(i2c, CS40L50_GPIO_CTRL1 + (4 * (gpio - 1)),
        CS40L50_GPIO_CTRL_DIR_BITMASK | CS40L50_GPIO_CTRL_FN_INPUT_OUTPUT);
}

static uint32_t cs40l50_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
    k_msleep(duration_ms);
    return 0;
}

static uint32_t cs40l50_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
    //TODO
    return 0;
}

static uint32_t cs40l50_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
    //TODO
    return 0;
}

bsp_driver_if_t cs40l50_bsp_driver_if = {
    .set_timer = cs40l50_set_timer,
    .register_gpio_cb = cs40l50_register_gpio_cb,
    .set_gpio = cs40l50_set_gpio
};

bsp_driver_if_t *bsp_driver_if_g = &cs40l50_bsp_driver_if;

/* ===================================================================================== */

static int cs40l50_init(const struct device *dev)
{
    struct cs40l50_config *config = dev->config;
    struct cs40l50_bsp *data = dev->data;
    cs40l50_t *drv = &data->priv;
    uint32_t val;
    int ret;
    LOG_INF("cs40l50_init\n");

    drv->config.bsp_config.i2c = &config->i2c;
    drv->config.syscfg_regs = cs40l50_syscfg_regs;
    drv->config.syscfg_regs_total = CS40L50_SYSCFG_REGS_TOTAL;
    drv->config.is_ext_bst = true;

    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_INF("cs40l50 no I2C\n");
        return -ENODEV;
    }

    ret = cs40l50_reset(drv);
    if (ret < 0) {
        return ret;
    }

    k_msleep(1000);

    regmap_read(&config->i2c, FIRMWARE_CS40L50_HALO_STATE, &val);
    LOG_INF("cs40l50_init: HALO_STATE = %x\n", val);

    LOG_INF("cs40l50_calibrate\n");
    ret = cs40l50_calibrate(drv);
    if (ret < 0) {
        LOG_INF("cs40l50_calibrate error\n");
        return ret;
    }

    ret = cs40l50_boot(drv, NULL);
    if (ret < 0) {
        LOG_INF("cs40l50_boot error\n");
        return ret;
    }

    cs40l50_setup_gpi(drv, 1);

    LOG_INF("cs40l50_firmware_load\n");
    ret = cs40l50_firmware_load(drv);
    if (ret < 0) {
        LOG_INF("cs40l50_firmware_load error\n");
        return ret;
    }

    ret = regmap_write(&config->i2c, CS40L50_DSP1_CCM_CORE_CONTROL, 0x00000281);
    if (ret < 0) {
        LOG_INF("cs40l50_boot error\n");
        return ret;
    }

    cs40l50_clear_gpio_triggers(drv);

    k_msleep(1000);

    /* to-do */
    regmap_read(&config->i2c, FIRMWARE_CS40L50_HALO_STATE, &val);
    LOG_INF("cs40l50_init: HALO_STATE = %x\n", val);

//    config->irq_cfg_func();
//    config->irq_enable_func();

    return 0;
}

static int haptics_cs40l50_stop_output(const struct device *dev)
{
    return 0;
}

static int haptics_cs40l50_start_output(const struct device *dev)
{
    struct cs40l50_bsp *data = dev->data;

    LOG_INF("haptics_cs40l50_start_output, bank=%x, index=%x\n", data->hap_cfg.bank, data->hap_cfg.index);

    cs40l50_trigger(&data->priv, data->hap_cfg.index, RAM_BANK);

    return 0;
}

static const struct haptics_driver_api cs40l50_driver_api = {
    .start_output = &haptics_cs40l50_start_output,
    .stop_output = &haptics_cs40l50_stop_output,
};

static void cs40l50_isr(void *arg)
{
}

#define CS40L50_INIT(inst)                                                                         \
                                                                                                   \
                                                                                                   \
    static const struct cs40l50_config cs40l50_config_##inst = {                               \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
    };                                                                                         \
                                                                                                   \
    static struct cs40l50_bsp cs40l50_bsp_data_##inst = {                                         \
                                                                                                   \
    };                                                                                         \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(inst, cs40l50_pm_action);                                         \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, &cs40l50_init, NULL, &cs40l50_bsp_data_##inst,                     \
                  &cs40l50_config_##inst, POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY,   \
                  &cs40l50_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS40L50_INIT)

/* ===================================================================================== */

