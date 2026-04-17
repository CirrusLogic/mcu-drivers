
# CS40L5X Zephyr Sample Application

## Introduction

This sample application illustrates how to integrate the CS40L5X SDK driver on a new microcontroller platform using the Zephyr OS and ST Nucleo F401RE as an example.

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/ for instructions on building the Zephyr samples.

## Goals

* Integrate SDK driver files
* Create a BSP that can use cs40l5x.c driver functions
* Create an application that can initialize the amplifier and run config scripts

# Source Files

_C files_
* cs40l5x/cs40l5x.c
* common/fw_img.c
* cs40l5x_bsp.c
* main.c


_Includes_
* cs40l5x/cs40l5x.h
* cs40l5x/cs40l5x_spec.h

_Firmware_
* cs40l5x_firmware.c
* cs40l5x_firmware.h

Firmware is generated using the firmware converter Python tool and a wavetable included in the SDK, run the following from the project/src directory, depending on the desired target part:


**CS40L51:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l51 ../../../../../cs40l5x/fw/CS40L51_Rev4.0.3.wmfw --wmdr ../../../../../cs40l5x/fw/cs40l5x_wt.bin
```

**CS40L52:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l52 ../../../../../cs40l5x/fw/CS40L52_Rev4.0.3.wmfw --wmdr ../../../../../cs40l5x/fw/cs40l5x_wt.bin
```

**CS40L53:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l53 ../../../../../cs40l5x/fw/CS40L53_Rev4.0.3.wmfw --wmdr ../../../../../cs40l5x/fw/cs40l5x_wt.bin
```

_Config_
* cs40l5x_syscfg_regs.c
* cs40l5x_syscfg_regs.h

Config is generated using the WISCE script converter Python tool and the wisce_init.txt included in the SDK (Run from **sample_app/** directory, not sample_app/src)

```
python  ../../../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l5x -i ../../../../cs40l5x/config/wisce_init.txt -o ../../../../samples/haptics/cs40l5x/sample_app/src/
```

# BSP

BSP Goals
* Link platform operations to driver (Control Port, GPIO)
* Initialize driver structs (cs40l5x_t, cs40l5x_bsp)
* Load firmware and application-specific data

## Control Port

The BSP driver instance is initialized with a `struct i2c_dt_spec` handle that can be used with Zephyr I2C devicetree operations (`include/zephyr/i2c.h`)

The BSP links the Zephyr I2C operations to the CS40L5X SDK driver by creating the wrapper functions:
* cs40l5x_i2c_read_reg_dt
* cs40l5x_i2c_write_reg_dt
* cs40l5x_update_reg_dt
* cs40l5x_write_array_dt
* cs40l5x_poll_reg_dt
* cs40l5x_write_acked_reg_dt

These can then be substituted for the corrsponding operations in the SDK demo board BSP.
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.h#L28-L35

## Timer

The BSP implements a `set_timer` callback using Zephyr's `k_msleep`.

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.c#L148-L152

## Instantiation

Once the BSP is linked to the SDK driver, the driver functions in `cs40l5x.c` can be used.
The entry point for the BSP/driver is the `cs40l5x_init` function.

The Zephyr init macros ensure that the BSP is allocated the data it needs (`cs40l5x_bsp` and `cs40l5x_config` structs), receives information about the I2C bus from devicetree (I2C_DT_SPEC_INST_GET), and starts in the function `cs40l5x_init()`.

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.c#L275-L292

## Firmware Loading

The BSP is responsible for loading the firmware. This is implemented in the `cs40l5x_firmware_load` and `cs40l5x_write_fw_blocks` functions.

Before loading firmware, call `cs40l5x_boot` with the `fw_img` param equal to `NULL` to disable the DSP.
After loading the firmware, boot the DSP by calling `cs40l5x_boot` with a firmware image parameter, or write to the DSP enable directly.

# Application

Application Goals
* Link BSP functions to system events

# Building Project
From the sample application directory, the project can be compiled using west, assuming the zephyr workspace has been setup according the the zephyr getting started guide. For each of the CS40L5X variants, a different devicetree file is used so they may share one i2c bus but be assigned different i2c addresses. To build each of the variants, run each of the following west commands at the zephyrproject directory level:

**CS40L51:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/sample_app/ -- -DCONFIG_HAPTICS_CS40L51=y -DDTC_OVERLAY_FILE="boards/cs40l51.overlay"
```

**CS40L52:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/sample_app/ -- -DCONFIG_HAPTICS_CS40L52=y -DDTC_OVERLAY_FILE="boards/cs40l52.overlay"
```

**CS40L53:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/sample_app/ -- -DCONFIG_HAPTICS_CS40L53=y -DDTC_OVERLAY_FILE="boards/cs40l53.overlay"
```

## Init

Due to the devicetree declaration and the CS40L5X_INIT definition, the BSP/driver will be instantiated when the system starts and will enter the cs40l5x_init function.

In main.c the application can check that the devicetree instantiation completed by making sure that DEVICE_DT_GET returned a valid `struct device *` pointer.

Initialization steps:
* Populate `cs40l5x_t` with I2C handle and syscfg registers
* Check I2C bus ready
* cs40l5x_reset()

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.c#L176-L197

## System events

The system uses a built-in Zephyr Haptic API.

The BSP implements these callbacks:
* haptics_cs40l5x_start_output
* haptics_cs40l5x_stop_output
