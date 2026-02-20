# CS35L56 Zephyr Sample Application

## Introduction

This sample application shows how to integrate the CS35L56 SDK driver with a zephyr bsp and sample application using the ST Nucleo-F401RE board as an example.

Please refer to https://github.com/CirrusLogic/mcu-drivers/tree/master/samples for instructions on building the Zephyr samples.


## Goals
* Integrate SDK driver files
* Create a zephyr BSP to use cs35l56.c driver functions
* Create an application to demonstrate firmware loading and ASP initialization, enabling pause/play of audio over i2s

# Source Files

_C files_
* cs35l56/cs35l56.c
* common/fw_img.c
* cs35l56_bsp.c
* main.c


_Includes_
* cs35l56/cs35l56.h
* cs35l56/cs35l56_spec.h

_Firmware_
* cs35l56_firmware.c
* cs35l56_firmware.h

Firmware is generated using the firmware converter Python tool and a wavetable included in the SDK
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs35l56 ../../../../../cs35l56/fw/CS35L56_Rev4.7.2.wmfw --wmdr ../../../../../cs35l56/fw/CS35L56_LT.bin ../../../../../cs35l56/fw/CS35L56_RT.bin --preserve-filename
```

# BSP

BSP Goals
* Link platform operations to driver (Control Port, GPIO)
* Initialize driver structs (cs35l56_t, cs35l56_bsp)
* Load firmware and application-specific data

## Control Port

The BSP driver instance is initialized with a `struct i2c_dt_spec` handle that can be used with Zephyr I2C devicetree operations (`include/zephyr/i2c.h`)

The BSP links the Zephyr I2C operations to the CS40L50 SDK driver by creating the wrapper functions:
* cs35l56_i2c_read_reg_dt
* cs35l56_i2c_write_reg_dt
* cs35l56_update_reg_dt
* cs35l56_write_array_dt
* cs35l56_poll_reg_dt
* cs35l56_write_acked_reg_dt

These can then be substituted for the corrsponding operations in the SDK demo board BSP.

## Instantiation

Once the BSP is linked to the SDK driver, the driver functions in `cs35l56.c` can be used.
The entry point for the BSP/driver is the `cs35l56_init` function.

The Zephyr init macros ensure that the BSP is allocated the data it needs (`cs35l56_bsp` and `cs35l56_config` structs), receives information about the I2C bus from devicetree (I2C_DT_SPEC_INST_GET), and starts in the function `cs35l56_init()`.

## Firmware Loading

The BSP is responsible for loading the firmware. This is implemented in the `cs35l56_firmware_load`,  `cs35l56_write_fw_blocks`, and `cs35l56_fw_reset` functions.

## Init

Due to the devicetree declaration and the CS35L56_INIT definition, the BSP/driver will be instantiated when the system starts and will enter the cs35l56_init function for each amp.

In main.c the application can check that each of the devicetree instantiations of the amps are completed by making sure that DEVICE_DT_GET returned a valid `struct device *` aointer.

Initialization steps:
* Populate `cs35l56_t` with I2C handle and syscfg registers
* Check I2C bus ready
* cs35l56_reset()
* Boot and load firmware with cs35l56_boot() and cs35l56_firmware_load()
* Enable ASP with settings for stereo audio streaming
* Pause audio playback

## ASP Enable

`cs35l56_set_asp_enable` is called in the BSP within the initialization entry point to enable or disable the ASP module and configure the PLL to specific refclk speed for stereo audio streaming.
