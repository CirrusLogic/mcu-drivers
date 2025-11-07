
# CS40L50 Zephyr Diagnostics Sample Application

## Introduction

This example shows usage of the diagnostic capabilities within the CS40L50 SDK driver.

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/ for instructions on building the Zephyr samples.

## Goals

* Create a BSP that can use cs40l50.c driver functions and includes wrapper APIs for the driver's diagnostic functionality
* Create an application that demonstrates utilizing these diagnostic APIs, including a configurable mask for diagnostic error flags

# Source Files

_C files_
* cs40l50/cs40l50.c
* common/fw_img.c
* cs40l50_bsp.c
* main.c


_Includes_
* cs40l50/cs40l50.h
* cs40l50/cs40l50_spec.h

_Firmware_
* cs40l50_firmware.c
* cs40l50_firmware.h

Firmware is generated using the firmware converter Python tool and a wavetable included in the SDK, run the following from the project directory:
```
python ../../../../tools/firmware_converter/firmware_converter.py export cs40l50 ../../../../cs40l50/fw/CS40L50_Rev3.4.14.wmfw --wmdr ../../../../cs40l50/fw/cs40l50_wt.bin --preserve-filename
```

_Config_
* cs40l50_syscfg_regs.c
* cs40l50_syscfg_regs.h

Config is generated using the WISCE script converter Python tool and the wisce_init.txt included in the SDK, run the following from the project directory:

```
python  ../../../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l50 -i ../../../../cs40l50/config/wisce_init.txt -o ../../../../samples/haptics/cs40l50/diagnostics/src/
```

# BSP

BSP Goals
* Link platform operations to driver (Control Port, GPIO, Diagnostics)
* Initialize driver structs
* Load firmware and application-specific data

# Application

Application Goals
* Link BSP functions to system events
* Perform Diagnostics on button1 press, printing detected diagnostic errors

## Init

Due to the devicetree declaration and the CS40L50_INIT definition, the BSP/driver will be instantiated when the system starts and will enter the cs40l50_init function.

In main.c the application can check that the devicetree instantiation completed by making sure that DEVICE_DT_GET returned a valid `struct device *` pointer.

Initialization steps:
* Populate `cs40l50_t` with I2C handle and syscfg registers
* Check I2C bus ready
* cs40l50_reset()

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l50/src/cs40l50_bsp.c#L176-L197

## System events

The system uses a built-in Zephyr Haptic API.

The BSP implements these callbacks:
* haptics_cs40l50_start_output
* haptics_cs40l50_stop_output
