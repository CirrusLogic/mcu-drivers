
# CS40L5X Zephyr Audio to Haptics Sample Application

## Introduction

This example builds upon the basic CS40L5X SDK driver sample application, showing how to enable the audio serial port (ASP) for continuous streaming.

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/ for instructions on building the Zephyr samples.

## Goals

* Load an Audio2Haptics tuning that can be used to test audio streaming to the LRA
* Create an application that can initialize the amplifier with ASP streaming support

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

Firmware is generated using the firmware converter Python tool and an A2H tuning included in the SDK, run the following from the project directory depending on the target part:


**CS40L51:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l51 ../../../../../cs40l5x/fw/CS40L51_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_A2H.bin ../tunings/cs40l5x_SVC.bin ../tunings/cs40l5x_AVC.bin
```

**CS40L52:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l52 ../../../../../cs40l5x/fw/CS40L52_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_A2H.bin ../tunings/cs40l5x_SVC.bin ../tunings/cs40l5x_AVC.bin
```

**CS40L53:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l53 ../../../../../cs40l5x/fw/CS40L53_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_A2H.bin ../tunings/cs40l5x_SVC.bin ../tunings/cs40l5x_AVC.bin
```

_Config_
* cs40l5x_syscfg_regs.c
* cs40l5x_syscfg_regs.h

Config is generated using the WISCE script converter Python tool and the wisce_init.txt included in the SDK, run the following from the project directory

```
python  ../../../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l5x -i ../../../../cs40l5x/config/wisce_init.txt -o ../../../../samples/haptics/cs40l5x/audio2haptics/src/
```
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.c#L148-L152

## Instantiation

Once the BSP is linked to the SDK driver, the driver functions in `cs40l5x.c` can be used.
The entry point for the BSP/driver is the `cs40l5x_init` function, which also now calls `cs40l5x_set_asp_enable()` in `cs40l5x.c`.

The Zephyr init macros ensure that the BSP is allocated the data it needs (`cs40l5x_bsp` and `cs40l5x_config` structs), receives information about the I2C bus from devicetree (I2C_DT_SPEC_INST_GET), and starts in the function `cs40l5x_init()`.

## Firmware Loading

The BSP is responsible for loading the firmware. This is implemented in the `cs40l5x_firmware_load` and `cs40l5x_write_fw_blocks` functions.

Before loading firmware, call `cs40l5x_boot` with the `fw_img` param equal to `NULL` to disable the DSP.
After loading the firmware, boot the DSP by calling `cs40l5x_boot` with a firmware image parameter, or write to the DSP enable directly.

# Application

Application Goals
* Link BSP functions to system events
* Setup ASP streaming functionality
* Perform playout from ROM wavtable when receiving serial data to demonstrate handling of overlapping ASP streaming/`haptics_start_output()` calls

# Building Project
From the sample application directory, the project can be compiled using west, assuming the zephyr workspace has been setup according the the zephyr getting started guide. The west command to build each of the variants will be as follows at the zephyrproject directory level:

**CS40L51:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/audio2haptics/ -- -DCONFIG_HAPTICS_CS40L51=y -DDTC_OVERLAY_FILE="boards/cs40l51.overlay"
```

**CS40L52:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/audio2haptics/ -- -DCONFIG_HAPTICS_CS40L52=y -DDTC_OVERLAY_FILE="boards/cs40l52.overlay"
```

**CS40L53:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/audio2haptics/ -- -DCONFIG_HAPTICS_CS40L53=y -DDTC_OVERLAY_FILE="boards/cs40l53.overlay"
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
