/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <sys/_stdint.h>

#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);

#define CS40L50_ADDR 0x34
#define CS40L50_DEVICE_ID 0x40A50
#define CS40L50_SYSCFG_REGS_TOTAL (44)

uint32_t cs40l50_syscfg_regs[] =
{   /* ADDR   VALUE */
    0x0040, 0x0055,
    0x0040, 0x00aa,
    0x3808, 0x40000001,
    0x38ec, 0x0032,
    0x0040, 0x0000,
    0x201c, 0x0010,
    0x3800, 0x026e,
    0x2034, 0x2000000,
    0x280279c, 0x0006,
    0x280285c, 0x0000,
    0x280404c, 0x40020,
    0x2804050, 0x1c0010,
    0x2804054, 0x40038,
    0x2804058, 0x02fa,
    0x280405c, 0xffffff,
    0x280404c, 0x50020,
    0x2804050, 0x340200,
    0x2804054, 0x40020,
    0x2804058, 0x183201,
    0x280405c, 0x50044,
    0x2804060, 0x40100,
    0x2804064, 0xffffff
};

int main(void)
{
        int ret, i;
        uint32_t val;
        uint8_t msg_buf[8], *addr_buf, *reg_buf;
        const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

        /* Check that I2C bus is ready */
        if (!i2c) {
                LOG_ERR("I2C bus not found");
                return -ENODEV;
        }

        addr_buf = msg_buf;
        reg_buf = msg_buf + 4;

        /* Read from CS40L50 device at register address 0 (device ID)*/
        sys_put_be32(0, addr_buf);
        ret = i2c_write_read(i2c, CS40L50_ADDR, addr_buf, 4, reg_buf, 4);
        val = sys_get_be32(reg_buf);

        if (ret != 0) {
                LOG_ERR("I2C Error (%d)", ret);
                return ret;
        } else if (val != CS40L50_DEVICE_ID) {
                LOG_ERR("CS40L50 device not found (%x)", val);
                return -ENODEV;
        }

        /* Apply syscfg registers */
        for (i = 0; i < CS40L50_SYSCFG_REGS_TOTAL; i++) {
                sys_put_be32(cs40l50_syscfg_regs[i++], addr_buf);
                sys_put_be32(cs40l50_syscfg_regs[i], reg_buf);
                ret = i2c_write(i2c, msg_buf, 8, CS40L50_ADDR);

                if (ret != 0) {
                        LOG_ERR("I2C Error (%d)", ret);
                        return ret;
                }
        }

        return 0;
}
