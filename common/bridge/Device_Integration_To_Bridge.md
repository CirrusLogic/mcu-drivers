# Device Integration to Bridge in Alt-OS

# 1 Brief System Description

Alt-OS has a Bridge implementation which is a Python3 script that runs on the host, and a Bridge Command Handler
that runs on the MCU. The Bridge allows WISCE/SoundClearStudio to read from and write to registers with Alt-OS
and hence the Cirrus device being controlled by Alt-OS. The Bridge supports a subset of the full StudioBridge
protocol as detailed in Appendix 1. Code on the MCU runs continuously to wait for and service Bridge commands
from the host, thus the Bridge is composed of two components as shown in the diagram below.

```
                                   MCU                                               PC HOST
                           +---------------------+                   +----------------------------------------+
                           |      Alt-OS         |                   |                         +-----------+  |
                           |                     |                   | +---------+             |           |  |
                           |+------------------+ |                   | |  Bridge |   TCP/IP    |  WISCE/   |  |
+----------+               ||       |          | |-------------------| |    &    |-------------|   SCS     |  |
|          |---------------||regmap |  Bridge  | | USB Serial V-COM  | |  SMCIO  |   <--Cmd    |           |  |
|  Device  |  SPI/I2C bus  ||       |  Cmd     | |-------------------| |         |   Rpl-->    |           |  |
|          |---------------||       |  Handler | |                   | +---------+             |           |  |
+----------+               |+------------------+ |                   |                         +-----------+  |
                           +---------------------+                   |                                        |
                                                                     +----------------------------------------+
```

## 1.1 Bridge on Host

This is a stand-alone Python3 script that needs to be started manually. It is found at tools/bridge_agent/
run_bridge.py. The main script makes use of bridge_agent.py which is a continuous loop for receiving commands
from WISCE/SCS and relaying them to the MCU. It sends responses from the MCU back to WISCE/SCS. The message
format between Bridge and MCU is coded and differs from the standard StudioBridge format.

The Bridge and WISCE/SCS communicate over a TCP/IP connection over port 22349. Currently the Bridge must run
on the same host as WISCE/SCS.

## 1.2 Serial channel and SMCIO
The connection between the Host and MCU depends on the application. The one used for development is a the
STLink USB connection, using a USB cable from Host to MCU; this exposes a virtual COM port on the Host.

The Bridge makes use of the SMCIO (Serial Multi-Channel IO) Python3 module (under tools/smcio) which creates
multiple virtual serial channels between the Host and MCU to help separate serial data packets according to their
type. This comes in use when there's a need to model, for example, separate channels for bridge data, stdout data
and stderr data. Code on both the Bridge/Host side and MCU Cmd Handler side have special SMCIO handling.

A customer will likely use their own communications link between Host and their MCU. Some notes on how to adapt
the Bridge and MCU code are given in the Appendix.

## 1.3 MCU Bridge Command Handler
The Bridge Command Handler at the MCU waits for Bridge commands and services them. This normally involves
doing a register read or write to the device via the Alt-OS regmap layer, and returning Success/Fail or data values
back to the Bridge. The regmap layer is configured for either SPI or I2C access to the device by the
REGMAP_BUS_TYPE_I2C and REGMAP_BUS_TYPE_SPI defined in common\regmap.h.

Only single commands are handled at any one time. Having more than one instance of the Bridge on the Host is not
supported. A client should wait for the response of the current command before issuing a new command.

## 1.4 Communications and Message Formats

### 1.4.1 WISCE/SCS to Bridge
The command and response message format between WISCE/SCS and the Bridge is defined by the BridgeProtocol
and is in ASCII.

### 1.4.2 Bridge to MCU
The command and response message format between Bridge and MCU is structured as Header:Payload:Footer. The
SMCIO layer deals with the Header and Footer. The payload carries the actual command and response.

The commands (sent from Bridge to MCU) have an encoded binary format. The responses (sent form MCU to Bridge)
have an ASCII format (this may change in future versions).

### 1.4.3 MCU to Device
The device sits on the MCU's SPI or I2C bus. The Alt-OS regmap layer can be configured to use either (see under
common/).

# 2 New Device Integration
This details the steps to integrate a Cirrus device to the Bridge implementation on Alt-OS.
The current list of Bridge integrated devices is (this list can change):
* CS47L63
* CS40L25
* CS35L41
* CS47L35

The below instructions show the steps to support the CS47L35 device.

# 3 Assumptions
* The device must already be supportd in the Alt-OS repository (check for the device's part number in the Alt-OS repository's root folder).
* The executable binary for the target MCU system and device under test can already be built from source for
either baremetal or freertos builds. Setting up this environment is not detailed here.
* If the device needs to be discoverable by WISCE or SCS, you need to have the device's WISCE device pack
and/or SCS BSP package installed. (The device may still be integrated by the Bridge and controlled via
StudioLink API tools without device packs but functionality may be severly limited. This is not detailed
further here).

# 4 Host Requirements
WISCE v3 and or SCS v2 must be installed on the host PC. Note that some older Cirrus devices are not supported by SCS. The
correct BSP device pack for WISCE or SCS must be installed.

The host must be connected to the MCU system via a virtual COM port.

Neither setting up the build environment nor flashing the MCU with the executable binary are covered here.


# 5 Code Changes
## 5.1 Baremetal Changes
This is an example of code changes for the baremetal build target

### 5.1.1 BSP Code

In the \<device\> folder:

In the Includes section include bridge.h to bsp/bsp_\<device\>.c

```C
#include "bridge.h"
```

In the Local Variables section add the device_list array. This declares the device to the bridge sub-system. The
below code assumes this device is the only one that exists on the bus

```C
static bridge_device_t device_list[] =
{
    {
    .bus_i2c_cs_address = 0, // See notes below
    .device_id_str = "6360", // Must contain within the value of register 0 in ASCII form eg "6360" for CS47L35 or "CS47A63" for CS47L63
    .dev_name_str = "CS47L35-CODEC", // Can be any user-friendly string with no spaces and of length MAX_BRIDGE_DEVICE_NAME_LEN (defined in common/bridg/bridge.h)
    .b.dev_id = BSP_DUT_DEV_ID, // See notes below
    .b.bus_type = REGMAP_BUS_TYPE_SPI, // Can be SPI or I2C, see definitions in common/regmap.h
    .b.receive_max = BRIDGE_BLOCK_BUFFER_LENGTH_BYTES,
    .b.spi_pad_len = 4 // Number of bytes to pad for SPI transactions
    },
};
```

---
**Notes:**

The device_list array contains a small set of attributes related to the device communication bus for which the
values depend heavily on the platform being used and the way the device is integrated to the platform. The above
example values are correct for the Cirrus internal platform used for Alt-OS development.

bus_i2c_cs_address: This field is used by the Bridge and WISCE/SCS clients and should be set to either the device's
I2C address as a hex value or the chip-select number for SPI bus-type. (If you are using a SCS Device Package then
you can get a hint of this value by looking at the \<DeviceAddress\> value in the systems' XML file.)

b.dev_id: This is an enumeration defined in common\platform_bsp\platform_bsp.h. The value is used by bus
access functions in platform_bsp.c to configure the correct pins to enable the correct bus lines for the device.

---

In the function bsp_dut_initialize add the following call to bridge_initialize near the end of the function but before
the return statement.

```C
bridge_initialize(&device_list[0], (sizeof(device_list)/sizeof(bridge_device_t)));
```

In the function bsp_dut_process add the following call to bridge_process near the end of the function but before
the return statement:

```C
bridge_process();
```

### 5.1.2 Makefile
Add the following two lines in the makefile of the device inside the "else ifdef IS_NOT_UNIT_TEST" clause. Do not
remove any of the other lines.

```
else ifdef IS_NOT_UNIT_TEST
 C_SRCS += $(COMMON_PATH)/bridge/bridge.c
 INCLUDES += -I$(COMMON_PATH)/bridge
endif
```

### 5.1.3 Building
Do the following

```bash
cd <repo root>
cd cs47l35
make clean; make baremetal [-jN] (N = number of CPU cores, for parallel build)
```

If successful a baremetal.elf binary is created in the build folder; you will see an output ending with:

```bash
-------------------------------------------------------------------------------
SIZE of baremetal.elf
arm-none-eabi-size -t xxxxxx/xxxxx/xxxx/driver/build/baremetal.elf
 text data bss dec hex filename
311940 544 20424 332908 5146c C:/Users/xxxxxx/xxxxx/xxxx/driver/build/baremetal.elf
311940 544 20424 332908 5146c (TOTALS)
```

The baremetal.elf file needs to be flashed to the MCU which is not covered here.

### 5.1.4 Testing
To test whether WISCE/SCS can detect the device we need to run the Alt-OS's Bridge Agent and start WISCE/SCS.

You must ensure the correct WISCE device pack or SCS system package has already been installed on your host
computer where WISCE or SCS is installed, otherwise WISCE/SCS will not be able to detect the device. (Installing
these tools is not covered here). Please note that some older devices are not supported by SCS; for example there is
no official SCS BSP package for CS47L35.

Ensure the MCU is running and is connected to the host PC via a USB virtual COM port. Ensure the COM port is
detected on the host PC (use Device manager on Windows and look under Ports (COM & LPT) for an entry). The
Bridge Agent will auto-detect the COM port of an STLink connection, however if some other vendor's connection is
being used you can try to specify the COM port number manually on the Agent command line.

Start the Bridge Agent as shown below. Note: currently, the Bridge Agent must run on the same host as WISCE/SCS.

```bash
cd <repo root>
cd tools/bridge_agent
python run_bridge.py (use -h argument to see Help, for example to specify a COM port)
```

The Bridge Agent should start and wait for a WISCE/SCS connection. The output should be something as below:
```bash
comport not supplied, attempting to autodetect ...
STLink auto-detected: COM4
Creating Socket to host: 127.0.0.1, port: 22349
Socket bind complete
Socket listening on port: 22349
```

Run WISCE and follow the steps to connect to a StudioBridge system (localhost, port 22349): File -> New -> System
-> Connect to a StudioBridge system -> Next.

Press Ok in the StudioBridge Connection dialog.

Running WISCE will generate output at the Agent similar to:

```bash
Socket connected

wisce_device_id is CS47L35-CODEC
Bridge and MCU msg format versions match: 0.1
device name to Id translation (used in MCU cmds): {'CS47L35-CODEC': 1}
```

You should now be able to interact with the device via WISCE.

## 5.2 FreeRTOS Code Changes
A few source files for freertos build are organised slightly differently to the baremetal build, and may still vary
slightly depending on the actual CODEC/device. These steps are an example of what changes are needed as applied
for the CS47L35 device. The main difference is that bridge_process() is to be called in its own thread.

### 5.2.1 FreeRTOS main
In the case of CS47L35 most of the code changes are made in cs47l35/freertos/main.c and cs47l35/bsp/
bsp_cs47l35.c.

Include the bridge.h file into cs47l35/freertos/main.c, as shown above in the baremetal section.

In file cs47l35/bsp/bsp_cs47l35.c, add the device_list array, as shown above in the baremetal section.

In file cs47l35/bsp/bsp_cs47l35.c, in the function bsp_dut_initialize call bridge_initialize, as shown above in the
baremetal section.

In file cs47l35/freertos/main.c, add the following in the LOCAL VARIABLES section:

```C
static TaskHandle_t BridgeTaskHandle = NULL;
```

Add the following in the LOCAL FUNCTIONS section:

```C
static void BridgeThread(void *argument)
{
    const TickType_t pollingTime = pdMS_TO_TICKS(5);
    while(true)
    {
        bridge_process();
        vTaskDelay(pollingTime);
    }
}
```

Search for where existing Tasks get created, usually inside the main function, add the following next to them:

```C
 xTaskCreate(BridgeThread,
             "BridgeTask",
             configMINIMAL_STACK_SIZE,
             (void *) NULL,
             tskIDLE_PRIORITY,
             &BridgeTaskHandle);
```

Ensure the above xTaskCreate comes before the call to vTaskStartScheduler()

### 5.2.2 Makefile
Refer to Makefile changes for Baremetal.

### 5.2.3 Building
Do the following

```bash
cd <repo root>
cd cs47l35
make clean; make freertos [-jN] (N = number of CPU cores, for parallel build)
```

If successful a freertos.elf binary is created in the build folder; you will see an output ending with:

```bash
SIZE of freertos.elf
arm-none-eabi-size -t /xxxx/xxxx/xxxx/driver/build/freertos.elf
 text data bss dec hex filename
321240 552 28960 350752 55a20 C:/Users/xxxx/xxxx/xxxx/driver/build/freertos.elf
321240 552 28960 350752 55a20 (TOTALS)
```

### 5.2.4 Testing
Testing is done in the same way as for the baremetal process.

# 6 Appendix 1

## 6.1 Alt-OS Bridge v0.1 Supported Messages

| StudioBridge Command  | Alt-OS Bridge v0.1 Support Status |
| --------------------- | ----------------------------------|
| CurrentDevice         |   Yes                             |
| ProtocolVersion       |   Yes                             |
| Info                  |   Yes                             |
| Detect                |   Yes                             |
| Read                  |   Yes                             |
| Write                 |   Yes                             |
| BlockRead             |   Yes                             |
| BlockWrite            |   Yes                             |
| Device                |   No                              |
| DriverControl         |   No                              |
| ServiceMessage        |   No                              |
| ServiceAvailable      |   No                              |
| Shutdown              |   No                              |


## 6.2 Adapting the Bridge and MCU Code
It is likely a customer will have a different means of communicating between the Bridge and MCU, and between the
MCU and device. These are hints at where to look in the code to help with the transition.

### 6.2.1 Bridge/SMCIO
The Bridge on the Host (tools/bridge_agent) uses the Cirrus developed SMCIO module that implements a multi-channel
UART which is a layer of channel abstraction over the Host's serial port (the virtual COM port created by the
STLink USB driver when the Host is attached to a running MCU).

The main class in tools/smcio.py is the processor class. This is responsible for sending and receiving packets to an
underlying physical serial communications port.


Class processor implements:
* The notion of separate Channels so that payloads can be sent to or received from a certain channel
    * Ability to create a new channel (add_channel) with unique Id
* receive and send message queues per channel, serviced by their own threads
* write_channel/write_channel_bytes and read_channel functions which are the high level API used by the Bridge (in bridge_agent.py) to send data to and receive data from the MCU on a certain channel
* packetising and de-packetising payload data

Class processor's implementation of write_channel/write_channel_bytes and read_channel handle all of the
sending and receiving of data to and from the MCU over an underlying serial communications port. The low level
serial coms port in the example Alt-OS code is the Host PC's virtual COM port created by the STLink/USB drivers. To
interface to the low level serial COM port the Bridge makes use of the 'serial' Python module.

The processor class constructor is passed an instance of class com_port which interfaces with the serial COM port
on the Host. Class com_port must implement read and write operations since it derives from class
smcio.serial_io_interface which is an abstract base class. The read and write methods in class com_port
fulfil this by reading and writing to the serial port on the Host.

To keep the higher level feaures offered by the SMCIO module but replace the lower layer serial COM port read and write, one could either replace the lower level serial read and write operations inside the com_port class, or derive
their own class from smcio.serial_io_interface all together. Either way should allow for a means of reimplemening the serial read and write operations.

If the communication between Host and MCU needs to be vastly different where the SMCIO layer is not suitable or is
not required, one should look to strip out calls to SMCIO inside run_bridge.py and then supply alternate
implementations for the following in bridge_agent.py:

* wait_for_serial_data
* where ever write_channel/write_channel_bytes and read_channel functions are called

The SMCIO module at the Bridge works closely with the Alt-OS MCU code, so MCU code will also need to be
modified.

### 6.2.2 MCU and Bridge
The Alt-OS MCU contains code to work with the SMCIO concept. The goal is to be able to use standard C file
operation calls such as fprintf and fgetc that work with the concept of separate files or channels. (On the SMCIO
host side the term channel is used whereas on the MCU side these are referred to as files, file-Ids or file descriptors).
In order to do this the Alt-OS MCU code made changes at the MCU's vendor supplied BSP layers in order to adapt a
few of the low level C system calls to work with the concept of a multi-channel UART.

#### 6.2.2.1 Disabling Multi-Channel UART
The compiler directive, CONFIG_USE_MULTICHANNEL_UART, is used to build support for a multi-channel UART (for
example in common\platform_bsp\eestm32int\platform_bsp.c this directive is used in many places to enable code
to support multi-channel UART structures).

The CONFIG_USE_MULTICHANNEL_UART compile directive can be disabled in a device's makefile (eg
cs47l63\makefile), however doing so will stop the MCU from receiving Bridge commands from the SMCIO module on
the Host. When CONFIG_USE_MULTICHANNEL_UART is disabled a single transmit channel (eg. for printf, fprintf, etc)
output data and a single receive channel (eg. for fread, fgetc, etc) input data remain.

Disabling both SMCIO (host) and multi-channel UART (MCU) will require Bridge commands be sent and
replied to on the single UART channel that remains, which is not a supported mode of function, or implementing
some other communications transport between Host and MCU for the Bridge to use.


#### 6.2.2.2 MCU and Device Bus
Alt-OS MCU code uses the regmap layer (common\regmap.c) to perform read and write operations to the Cirrus device.
SPI and I2C bus types are supported as defined in common\regmap.h.

Bridge Command Handler code (common\bridge\bridge.c) calls the regmap service to carry out Bridge commands.
There are helpful comments in bridge.c highlighting when regmap access is being made to assist in replacing
them with other bus access mechanisms.
