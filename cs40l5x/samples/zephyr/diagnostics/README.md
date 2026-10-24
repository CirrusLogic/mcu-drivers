
# CS40L5X Zephyr Diagnostics Sample Application

## Introduction

This example shows usage of the diagnostic capabilities within the CS40L5X SDK driver.

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/ for instructions on building the Zephyr samples.

## Goals

* Create a BSP that can use cs40l5x.c driver functions and includes wrapper APIs for the driver's diagnostic functionality
* Create an application that demonstrates utilizing these diagnostic APIs, including a configurable mask for diagnostic error flags

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

Firmware is generated using the firmware converter Python tool and a wavetable included in the SDK, run the following from the project directory:

**CS40L51:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l51 ../../../../../cs40l5x/fw/CS40L51_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_wt.bin
```
**CS40L52:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l52 ../../../../../cs40l5x/fw/CS40L52_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_wt.bin
```
**CS40L53:**
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l53 ../../../../../cs40l5x/fw/CS40L53_Rev4.0.7.wmfw --wmdr ../tunings/cs40l5x_wt.bin
```

_Config_
* cs40l5x_syscfg_regs.c
* cs40l5x_syscfg_regs.h

Config is generated using the WISCE script converter Python tool and the wisce_init.txt included in the SDK, run the following from the project directory:

```
python  ../../../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l5x -i ../../../../cs40l5x/config/wisce_init.txt -o ../../../../samples/haptics/cs40l5x/diagnostics/src/
```

# BSP

BSP Goals
* Link platform operations to driver (Control Port, GPIO, Diagnostics)
* Initialize driver structs
* Load firmware and application-specific data

# Building Project
From the sample application directory, the project can be compiled using west, assuming the zephyr workspace has been setup according the the zephyr getting started guide. The west command to build each of the variants will be as follows at the zephyrproject directory level:

**CS40L51:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/diagnostics/ -- -DCONFIG_HAPTICS_CS40L51=y -DDTC_OVERLAY_FILE="boards/cs40l51.overlay"
```

**CS40L52:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/diagnostics/ -- -DCONFIG_HAPTICS_CS40L52=y -DDTC_OVERLAY_FILE="boards/cs40l52.overlay"
```

**CS40L53:**
```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/diagnostics/ -- -DCONFIG_HAPTICS_CS40L53=y -DDTC_OVERLAY_FILE="boards/cs40l53.overlay"
```

# Application


This application performs diagnostics on button1 press, printing detected diagnostic errors as well as the masked diagnostics flag result. The CS40L5X diagnostics algorithm will check that data is within given given boundaries for each test in diagnostics. This functionality can be checked by defining `TEST_DIAGNOSTICS_EXTREMES` in `cs40l5x_bsp.h`. This will enable a call to function `haptics_cs40l5x_test_diagnostics(dev1, low_extreme_test)` before the diagnostic function in the main loop of the sample application. Here the lower boundaries can be checked by setting `low_extreme_test = true` and the high boundaries can be checked by setting `low_extreme_test = false`.

## Init

Due to the devicetree declaration and the CS40L5X_INIT definition, the BSP/driver will be instantiated when the system starts and will enter the cs40l5x_init function.

In main.c the application can check that the devicetree instantiation completed by making sure that DEVICE_DT_GET returned a valid `struct device *` pointer.

Initialization steps:
* Populate `cs40l5x_t` with I2C handle and syscfg registers
* Toggle GPIOA 6 (system reset) to reset the Lochnagar
* Check I2C bus ready
* cs40l5x_reset()

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l5x/src/cs40l5x_bsp.c#L176-L197
