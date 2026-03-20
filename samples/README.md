# Zephyr Sample Applications

Follow the Zephyr Getting Started guide to create a Zephyr workspace: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

The SDK samples will use the ST Nucleo F401RE board as a target (nucleo_f401re)

## Directory structure

Clone the SDK repo (mcu-drivers) inside the 'zephyrproject' directory, at the same level as the Zephyr kernel. This repo includes a samples directory with sample zephyr applications for the drivers.
```
└── zephyrproject
    ├── bootloader
    ├── build
    ├── mcu-drivers
        └── samples
    ├── modules
    ├── tools
    └── zephyr
```

## Building

Use west when in the zephyrproject directory to build a sample application (located in samples) by providing the path to the sample directory inside the SDK
```
west build -p always -b nucleo_f401re mcu-drivers/samples/haptics/cs40l50/sample_app
```

Some sample applications may have additional project configuration parameters. These can be passed using `-- -DCONFIG_SOME_PARAMETER=y` into the west build command. These build arguments are described in further detail in the individual sample application README files.

An example is shown below for building the CS40L5X host initiated haptics sample application targetting the CS40L52 chip.

```
west build -p always -b nucleo_f401re driver/samples/haptics/cs40l5x/host_init_haptics/ -- -DCONFIG_HAPTICS_CS40L52=y
```
## Samples CS40L50

Hello World:
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/helloworld/cs40l50/



Basic Sample Application:
https://github.com/CirrusLogic/mcu-drivers/tree/master/samples/haptics/cs40l50/sample_app



Using Diagnostics:
https://github.com/CirrusLogic/mcu-drivers/tree/master/samples/haptics/cs40l50/diagnostics


Audio to Haptics:
https://github.com/CirrusLogic/mcu-drivers/tree/master/samples/haptics/cs40l50/audio2haptics


Host Initiated Haptics
https://github.com/CirrusLogic/mcu-drivers/tree/master/samples/haptics/cs40l50/host_init_haptics
