
# CS40L50 Zephyr Host Initiated Haptics Sample Application

## Introduction

This projects demonstrates the use of Open Wave Table (OWT) API functions to create a sample application for the host initiated haptics requirements described here: https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/input-haptics-implementation-guide#host-initiated-haptic-feedback

Please refer to https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/ for instructions on building the Zephyr samples.

## Goals

* Create a BSP that can use cs40l50.c OWT driver functions and includes wrapper APIs to allow the user to easily write and trigger effects in the OWT.
* Create a shell application that uses these BSP wrappers to allow the user to list available effects with their respective durations,
and trigger these effects with configurable intensity, repeats, and repeat delay period.

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

Firmware is generated using the firmware converter Python tool and a wavetable included in the SDK, run the following from the project/src directory:
```
python ../../../../../tools/firmware_converter/firmware_converter.py export cs40l50 ../../../../../cs40l50/fw/CS40L50_Rev4.0.3.wmfw --wmdr ../../../../../cs40l50/fw/host_init_haptics.bin ../../../../../cs40l50/fw/SVC_FW_4.0.3.bin
```
_Waveforms_
* waveforms.c
* waveforms.h

Waveform source files are generated using the hwt converter python tool and a .hwt file corresponding to the wavetable included in the SDK, run the follow from the project/src directory:
```
python ../../../../../tools/hwt_to_waveform/hwt_to_waveform_converter.py  cs40l50 ../../../../../cs40l50/fw/host_init_haptics.hwt
```

_Config_
* cs40l50_syscfg_regs.c
* cs40l50_syscfg_regs.h

Config is generated using the WISCE script converter Python tool and the wisce_init.txt included in the SDK, run the following from the project directory:

```
python  ../../../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l50 -i ../../../../cs40l50/config/wisce_init.txt -o ../../../../samples/haptics/cs40l50/host_init_haptics/src/
```

# BSP

BSP Goals
* Link OWT API from driver (OWT write/trigger functions)
* Initialize driver structs
* Load firmware and application-specific data

# Application

Application Goals
* Link BSP functions to system events
* Enable host initiated haptic triggers based on user input arguments

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
