
# CS40L50 Zephyr Sample Application

## Introduction

This is a simple application for bringup and hardware verification.
It does not depend on any SDK source.

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/README.md for instructions on building the Zephyr samples.

## Goals

* Verify control port access
* Intitialize the device based on a predefined script

# Source Files

C files to build
* main.c

# Syscfg regs

A set of register writes to initialize the part is created with the WISCE script converter tool and the wisce_init.txt included in the SDK

```
python  tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l50 -i cs40l50/config/wisce_init.txt -o samples/haptics/cs40l50/src/
```

The array `cs40l50_syscfg_regs` and the array size `CS40L50_SYSCFG_REGS_TOTAL` are copied from the output files
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/helloworld/cs40l50/src/main.c#L22-L48

# Hardware

A CS40L50 device is connected to the STM32 F401RE I2C bus labelled "i2c1" in devicetree.
The I2C bus is enabled in the devicetree overlay and the USART that shares the pins is disabled.
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/helloworld/cs40l50/boards/nucleo_f401re.overlay#L1-L17

# Application

The application has three steps:
* Check that the I2C bus is ready
* Check that the device ID can be read from address 0
* Load syscfg registers

https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/helloworld/cs40l50/src/main.c#L57-L88
