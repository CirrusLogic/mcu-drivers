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

#include "cs35l56_bsp.h"
#include "cs35l56.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>

#include "cs35l56_firmware.h"

LOG_MODULE_REGISTER(cirrus_cs35l56);

struct cs35l56_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec reset_gpio;
    void (*irq_cfg_func)(void);
    void (*irq_enable_func)(void);
    void (*irq_disable_func)(void);
};

int cs35l56_i2c_write_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                 const uint32_t value)
{
    uint8_t addr_buf[CS35L56_ADDR_BYTES], data_buf[CS35L56_DATA_BYTES],
        msg_buf[CS35L56_I2C_MSG_BYTES];
    int i, j = CS35L56_DATA_BYTES;

    sys_put_be32(reg_addr, addr_buf);
    sys_put_be32(value, data_buf);

    for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
        msg_buf[i] = addr_buf[i];
        msg_buf[j] = data_buf[i];
        j++;
    }

    return i2c_write_dt(spec, msg_buf, sizeof(msg_buf));
}

int cs35l56_i2c_read_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                uint32_t *value)
{
    uint8_t write_buf[CS35L56_DATA_BYTES], read_buf[CS35L56_DATA_BYTES];
    int ret;

    sys_put_be32(reg_addr, write_buf);

    ret = i2c_write_read_dt(spec, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf));
    if (ret < 0) {
        LOG_INF("cs35l56_i2c_read_reg_dt error\n");
        return ret;
    }

    *value = sys_get_be32(read_buf);

    return 0;
}

int cs35l56_update_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
              const uint32_t mask, const uint32_t value)
{
    uint32_t old_value, new_value;
    int ret;

    ret = cs35l56_i2c_read_reg_dt(spec, reg_addr, &old_value);
    if (ret < 0) {
        return ret;
    }

    new_value = (old_value & ~mask) | (value & mask);
    if (new_value == old_value) {
        return 0;
    }

    return cs35l56_i2c_write_reg_dt(spec, reg_addr, new_value);
}

/* write addr/word pairs */
int cs35l56_write_array_dt(const struct i2c_dt_spec *spec, const uint32_t *array, uint32_t words)
{
    int i, ret;

    for (i = 0; i < words; i += 2) {
        ret = cs35l56_i2c_write_reg_dt(spec, array[i], array[i + 1]);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

/**
 * Reads a register for a specific value for a specified amount of tries while waiting between
 * reads.
 *
 */
int cs35l56_poll_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr, uint32_t value,
            uint32_t tries, uint32_t delay)
{
    uint32_t tmp;
    int i, ret;

    for (i = 0; i < tries; i++) {
        ret = cs35l56_i2c_read_reg_dt(spec, reg_addr, &tmp);
        if (ret < 0) {
            return ret;
        }

        if (tmp == value) {
            return 0;
        }

        k_msleep(delay);
    }
    return 0;
}

int cs35l56_write_acked_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t val, uint32_t acked_val, uint8_t tries, uint32_t delay)
{
    int i, ret;

    ret = cs35l56_i2c_write_reg_dt(spec, reg_addr, val);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < tries; i++) {
        uint32_t temp_reg_val;

        k_msleep(delay);

        cs35l56_i2c_read_reg_dt(spec, reg_addr, &temp_reg_val);

        if (temp_reg_val == acked_val) {
            return 0;
        }
    }

    return -1;
}

int cs35l56_i2c_write_bulk_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   const uint8_t *buf, unsigned int num_bytes, uint16_t bus_addr)
{
    uint8_t addr_buf[CS35L56_ADDR_BYTES], *msg_buf;
    int i, ret = 0;

    LOG_INF("cs35l56_i2c_write_bulk_bus: reg=%x size=%d\n", reg_addr, num_bytes);

    msg_buf = k_malloc(num_bytes + CS35L56_DATA_BYTES);
    if (!msg_buf) {
        return -ENOMEM;
    }

    sys_put_be32(reg_addr, addr_buf);

    for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
        msg_buf[i] = addr_buf[i];
    }

    bytecpy(msg_buf + CS35L56_DATA_BYTES, buf, num_bytes);

    ret = i2c_write(spec->bus, msg_buf, CS35L56_DATA_BYTES + num_bytes, bus_addr);

    k_free(msg_buf);
    return ret;
}

int cs35l56_i2c_write_bus(const struct i2c_dt_spec *spec, const uint32_t reg_addr, uint32_t val,
              uint16_t bus_addr)
{
    uint8_t addr_buf[CS35L56_ADDR_BYTES], data_buf[CS35L56_DATA_BYTES], *msg_buf;
    int ret = 0;

    LOG_INF("cs35l56_i2c_write_bus: reg=%x\n", reg_addr);

    msg_buf = k_malloc(CS35L56_I2C_MSG_BYTES);
    if (!msg_buf) {
        return -ENOMEM;
    }

    sys_put_be32(reg_addr, addr_buf);
    sys_put_be32(val, data_buf);

    bytecpy(msg_buf, addr_buf, CS35L56_ADDR_BYTES);
    bytecpy(msg_buf + CS35L56_ADDR_BYTES, data_buf, CS35L56_DATA_BYTES);

    ret = i2c_write(spec->bus, msg_buf, CS35L56_I2C_MSG_BYTES, bus_addr);

    k_free(msg_buf);
    return ret;
}

int cs35l56_i2c_write_bulk_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                  const uint8_t *buf, unsigned int num_bytes)
{
    uint8_t addr_buf[sizeof(reg_addr)];
    struct i2c_msg msg[2];

    LOG_INF("%s: reg=0x%x size=%d", __func__, reg_addr, num_bytes);

    sys_put_be32(reg_addr, addr_buf);

    msg[0].buf = addr_buf;
    msg[0].len = CS35L56_DATA_BYTES;
    msg[0].flags = I2C_MSG_WRITE;

    msg[1].buf = (uint8_t *)buf;
    msg[1].len = num_bytes;
    msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    return i2c_transfer_dt(spec, msg, 2);
}

static int cs35l56_write_fw_blocks(const struct i2c_dt_spec *i2c, halo_boot_block_t *blocks,
                   int num_blocks)
{
    int i, ret;
    halo_boot_block_t block;
    uint32_t bytes, address;
    uint8_t *buffer;

    for (i = 0; i < num_blocks; i++) {
        block = blocks[i];
        bytes = block.block_size;
        address = block.address;
        buffer = (uint8_t *)block.bytes;
        ret = cs35l56_i2c_write_bulk_dt(i2c, address, buffer, bytes);
        if (ret != 0) {
            return ret;
        }
        k_msleep(5);
    }

    return 0;
}

static int cs35l56_fw_reset(const struct device *dev)
{
    int i = 0, ret;
    uint32_t val;

    struct cs35l56_bsp *data = dev->data;
    cs35l56_t *drv = &data->priv;
    const struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    ret = regmap_write(i2c, FIRMWARE_CS35L56_HALO_STATE, CS35L56_DSP_STATE_HIBERNATE);
    if (ret < 0) {
        return ret;
    }

    ret = regmap_write(i2c, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_SYSTEM_RESET);
    if (ret < 0) {
        return ret;
    }

    while (i < 50) {
        ret = regmap_read(i2c, CS35L56_DSP_VIRTUAL1_MBOX_1, &val);
        if (ret < 0) {
            return ret;
        }

        if (val == 0) {
            return 0;
        }

        k_usleep(1000);
        i++;
    }

    return -ETIME;
}

static int cs35l56_firmware_load(const struct device *dev)
{
    int num_blocks, ret;
    halo_boot_block_t *blocks;
    struct cs35l56_bsp *data = dev->data;
    cs35l56_t *drv = &data->priv;
    const struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;

    num_blocks = cs35l56_total_fw_blocks;
    blocks = (halo_boot_block_t *)cs35l56_fw_blocks;
    ret = cs35l56_write_fw_blocks(i2c, blocks, num_blocks);
    if (ret) {
        return ret;
    }

    if (strcmp(dev->name, "cs35l56@0x30") == 0) {
        LOG_INF("cs35l56_firmware_load: Loading Left Amp Tuning\n");

        num_blocks = CS35L56_LT_total_coeff_blocks;
        blocks = (halo_boot_block_t *)CS35L56_LT_coeff_blocks;
        ret = cs35l56_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret) {
            return ret;
        }
    }
    if (strcmp(dev->name, "cs35l56@0x31") == 0) {
        LOG_INF("cs35l56_firmware_load: Loading Right Amp Tuning\n");

        num_blocks = CS35L56_RT_total_coeff_blocks;
        blocks = (halo_boot_block_t *)CS35L56_RT_coeff_blocks;
        ret = cs35l56_write_fw_blocks(i2c, blocks, num_blocks);
        if (ret) {
            return ret;
        }
    }

    ret = cs35l56_fw_reset(dev);
    if (ret) {
        return ret;
    }

    LOG_INF("Loaded tuning set");
    return 0;
}

static uint32_t cs35l56_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
    k_msleep(duration_ms);
    return 0;
}

static uint32_t cs35l56_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
    // TODO
    return 0;
}

static uint32_t cs35l56_set_gpio(uint32_t gpio_input, uint8_t gpio_state)
{
    uint32_t ret;
    const struct gpio_dt_spec *gpio_id = (const struct gpio_dt_spec *)gpio_input;
    if (gpio_state == 0) {
        printk("Deactivate GPIO\n");
        ret = gpio_pin_set_dt(gpio_id, BSP_GPIO_ACTIVE);
    } else {
        printk("Activate GPIO\n");
        ret = gpio_pin_set_dt(gpio_id, BSP_GPIO_INACTIVE);
    }
    return ret;
}

bsp_driver_if_t cs35l56_bsp_driver_if = {.set_timer = cs35l56_set_timer,
                     .register_gpio_cb = cs35l56_register_gpio_cb,
                     .set_gpio = cs35l56_set_gpio};

bsp_driver_if_t *bsp_driver_if_g = &cs35l56_bsp_driver_if;

static int cs35l56_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
    // TODO
    return 0;
}

static void cs35l56_start_output(const struct device *dev)
{
    int ret = 0;
    struct cs35l56_bsp *data = dev->data;
    cs35l56_t *drv = &data->priv;
    const struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    ret = regmap_write(i2c, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PLAY);
    if (ret) {
        LOG_ERR("Failed to start output.");
    }
}

static void cs35l56_stop_output(const struct device *dev)
{
    int ret = 0;
    struct cs35l56_bsp *data = dev->data;
    cs35l56_t *drv = &data->priv;
    const struct i2c_dt_spec *i2c = drv->config.bsp_config.i2c;
    ret = regmap_write(i2c, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PAUSE);
    if (ret) {
        LOG_ERR("Failed to stop output.");
    }
}

static int cs35l56_init(const struct device *dev)
{
    const struct cs35l56_config *config = dev->config;
    struct cs35l56_bsp *data = dev->data;
    cs35l56_t *drv = &data->priv;
    uint32_t val;
    int ret;
    LOG_INF("cs35l56_init\n");

    // Setup reset pin
    drv->config.bsp_config.i2c = &config->i2c;
    drv->config.is_ext_bst = true;
    static const struct gpio_dt_spec reset =
        GPIO_DT_SPEC_GET(DT_NODELABEL(cs35l56_l), reset_gpios);

    if (!gpio_is_ready_dt(&reset)) {
        LOG_INF("cs35l56 reset pin GPIO port is not ready.\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&reset, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        LOG_INF("cs35l56 configuring reset pin failed: %d\n", ret);
        return 0;
    }

    drv->config.bsp_config.reset_gpio_id = &reset;

    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_INF("cs35l56 no I2C\n");
        return -ENODEV;
    }

    ret = cs35l56_reset(drv);
    if (ret < 0) {
        LOG_INF("cs35l56 reset failed: %d\n", ret);
        return ret;
    }

    regmap_read(&config->i2c, FIRMWARE_CS35L56_HALO_STATE, &val);
    LOG_INF("cs35l56_init: HALO_STATE = %x\n", val);

    ret = cs35l56_boot(drv, NULL);
    if (ret < 0) {
        LOG_INF("cs35l56_boot error\n");
        return ret;
    }

    LOG_INF("cs35l56_firmware_load\n");
    ret = cs35l56_firmware_load(dev);
    if (ret < 0) {
        LOG_INF("cs35l56_firmware_load error\n");
        return ret;
    }

    LOG_INF("cs35l56_init: Enable ASP w/ %dHz refclk freq",
        AUDIO_PCM_RATE_48K * AUDIO_PCM_WIDTH_32_BITS * 2);
    ret = cs35l56_set_asp_enable(drv, true,
                     AUDIO_PCM_RATE_48K * AUDIO_PCM_WIDTH_32_BITS *
                         2); // Enable PPL with 3.072 MHz refclk frequency
    if (ret) {
        LOG_INF("cs35l56_set_asp_enable error");
        return ret;
    }
    k_msleep(1000);

    regmap_read(&config->i2c, CS35L56_SW_RESET_DEVID_REG, &val);
    LOG_INF("cs35l56_init: DEVICE ID: = %x\n", val);

    regmap_read(&config->i2c, FIRMWARE_CS35L56_HALO_STATE, &val);
    LOG_INF("cs35l56_init: HALO_STATE = %x\n", val);

    return 0;
}

static const struct audio_codec_api cs35l56_driver_api = {
    .configure = cs35l56_configure,
    .start_output = cs35l56_start_output,
    .stop_output = cs35l56_stop_output,
};

#define CS35L56_INIT(inst)                                                                         \
                                                                                                   \
    static const struct cs35l56_config cs35l56_config_##inst = {                               \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
        .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
    };                                                                                         \
                                                                                                   \
    static struct cs35l56_bsp cs35l56_bsp_data_##inst = {                                      \
                                                                                                   \
    };                                                                                         \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(inst, cs35l56_pm_action);                                         \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, cs35l56_init, NULL, &cs35l56_bsp_data_##inst,                  \
                  &cs35l56_config_##inst, POST_KERNEL,                                 \
                  CONFIG_AUDIO_CODEC_INIT_PRIORITY, &cs35l56_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS35L56_INIT)
