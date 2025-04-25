# Zephyr Sample Applications

Follow the Zephyr Getting Started guide to create a Zephyr workspace: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

The SDK samples will use the ST Nucleo F401RE board as a target (nucleo_f401re)

## Directory structure

Clone the SDK repo (mcu-drivers) inside the 'zephyrproject' directory, at the same level as the Zephyr kernel
```
└── zephyrproject
    ├── bootloader
    ├── build
    ├── mcu-drivers
    ├── modules
    ├── tools
    └── zephyr
```

## Building

Use west to build the sample by providing the path to the sample directory inside the SDK
```
west build -p always -b nucleo_f401re mcu-drivers/samples/haptics/cs40l50
```

## Samples

Hello World:
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/helloworld/cs40l50/


SDK Integration:
https://github.com/CirrusLogic/mcu-drivers/blob/7faad294f130f55ace18f95e3f3ed827589601cd/samples/haptics/cs40l50/
