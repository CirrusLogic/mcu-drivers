/**
 * @file bridge.c
 *
 * @brief The bridge implementation for WISCE or SCS interaction
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
// Standard include files
#include <string.h>  // strncmp, strlen, strcat, strcpy
#include <stdlib.h>  // atoi, strtoul
#include <stdio.h>   // sscanf

// Local include files
#include "bridge.h"
#include "platform_bsp.h"
#ifdef CONFIG_USE_VREGMAP
#include "vregmap.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define CHAR_RX_LEN                 (2)
#define MAX_BLOCK_DATA_BYTES        (BRIDGE_MAX_BLOCK_READ_BYTES * CHAR_RX_LEN)
#define MSG_RX_LEN                  (MAX_BLOCK_DATA_BYTES + 50)
#define MSG_TX_LEN                  (MAX_BLOCK_DATA_BYTES + 1)
#if (MSG_RX_LEN > MSG_TX_LEN)
    #define CMD_RESP_LENGTH_CHAR    (MSG_RX_LEN)
#else
    #define CMD_RESP_LENGTH_CHAR    (MSG_TX_LEN)
#endif

/* Protocol Commands v1.6 (106) Protocol Doc WTN_0381
   Note the MCU expects a simplified cut-down version of Cmds literals
   from the Agent
*/
#define PROTOCOLVERSION   ("PV")        // "ProtocolVersion"
#define INFO              ("IN")        // "INFO"
#define READ              ("RE")        // "Read"
#define WRITE             ("WR")        // "Write"
#define BLOCKREAD         ("BR")        // "BlockRead"
#define BLOCKWRITE        ("BW")        // "BlockWrite"
#define DETECT            ("DT")        // "Detect"
#define DEVICE            ("DV")        // "Device"
#define DRIVERCONTROL     ("DC")        // "DriverControl"
#define SERVICEMESSAGE    ("SM")        // "ServiceMessage"
#define SERVICEAVAILABLE  ("SA")        // "ServiceAvailable"
#define SHUTDOWN          ("SD")        // "Shutdown"
// Internal
#define BLOCKWRITE_START  ("BWs")        // "BlockWrite chunk start"
#define BLOCKWRITE_CONT   ("BWc")        // "BlockWrite chunk continue"
#define BLOCKWRITE_END    ("BWe")        // "BlockWrite chunk end"
#define CURRENT_DEVICE    ("CD")


/* Error Codes
 23 (WMT_INVALID_PARAMETER) - Encountered an unexpected null pointer in the server.
 27 (WMT_WRITE_FAILED) - Failed to write debug control value
 28 (WMT_READ_FAILED) - Unable to parse the register line from the codec file.
    Either the device is in low power mode or the line is in an unrecognized format.
 32 (WMT_RESOURCE_FAIL) - Failed to allocate memory.
 33 (WMT_UNSUPPORTED) - Operation is not supported by the current StudioBridge implementation.
 36 (WMT_NO_DEVICE) - No device present or failed to open codec file.
 37 (WMT_REG_NOT_PRESENT) - Register is not present on device.
 46 (WMT_TRUNCATED) - Successfully read from the codec file, but the given buffer
    was not large enough for the requested count of bytes to be read - data has been truncated.
 63 General failure - String manipulation error in the server, failed to read from the
    codec file or there was a failure when communicating with the device.
 1E (WMT_INVALID_COMMAND) - Missing <reg> value or <reg> is too long

*/
// These represent Hex values as string literals
#define WMT_INVALID_COMMAND     ("1E")
#define WMT_INVALID_PARAMETER   ("23")
#define WMT_WRITE_FAILED        ("27")
#define WMT_READ_FAILED         ("28")
#define WMT_RESOURCE_FAIL       ("32")
#define WMT_UNSUPPORTED         ("33")
#define WMT_NO_DEVICE           ("36")
#define WMT_REG_NOT_PRESENT     ("37")
#define WMT_TRUNCATED           ("46")
#define GENERAL_FAILURE         ("63")
#define EVERYTHING_IS_OK        ("0")

#define ERROR                   ("ER")

// Some defines used in responses, for Shelley
/* So on Alt-OS need a Info table coded for THAT device in file bsp_<device>.c
 where we read all this info from, incl if multi-chip device and device identifiers
 */
#define APP_NAME        ("\"StudioBridge\"")
#define APP_VER         ("\"1.5.13.0\"")   // copied from protocol doc. Adjust
#define PROTO_VER       ("\"106\"")
#define STR(A) #A
#ifdef SYS_ID
#define STR_X(A) STR(A)
#define SYSTEM_ID STR_X(SYS_ID)
#else
#define SYSTEM_ID       STR(DEADBEEF)
#endif
#define OP_SYS          ("\"Alt-OS\"")
#define OP_SYS_VER      ("\"0.0.0\"")
#define DRIVER_CTRL     ("false")

#define WRITE_OK        ("Ok")

// Hacky way to know we are a single chip device
#define NUM_CHIPS       (1)   // Eventually get this from new structure

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef struct
{
    bridge_device_t *device_list;
    uint8_t num_devices;
    bridge_device_t *current_device;
} bridge_t;

typedef uint32_t (*bridge_command_handler_t)(char *cmd);

typedef struct
{
    const char *cmd;
    bridge_command_handler_t handler;
} bridge_command_handler_map_t;

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static char cmd_resp[CMD_RESP_LENGTH_CHAR] = {0};
static uint8_t block_buffer[BRIDGE_BLOCK_BUFFER_LENGTH_BYTES] = {0};
static const char bus_name_i2c[] = "I2C";
static const char bus_name_spi[] = "SPI";
#ifdef CONFIG_USE_VREGMAP
static const char *bus_name_vregmap = bus_name_i2c;
#endif
static bridge_t bridge;

static uint32_t bw_chip_num = 0;
static uint32_t bw_addr = 0;
static uint32_t bw_data_collect_indx = 0;

static uint32_t handle_protocol_version(char *cmd);
static uint32_t handle_info(char *cmd);
static uint32_t handle_read(char *cmd);
static uint32_t handle_write(char *cmd);
static uint32_t handle_blockread(char *cmd);
static uint32_t handle_blockwrite_start(char *cmd);
static uint32_t handle_blockwrite_cont(char *cmd);
static uint32_t handle_blockwrite_end(char *cmd);
static uint32_t handle_detect(char *cmd);
static uint32_t handle_unsupported(char *cmd);
static uint32_t handle_invalid(char *cmd);
static uint32_t handle_current_device(char *cmd);

// An array of coded command Ids mapped to their handler functions
static const bridge_command_handler_map_t command_handler_map[] =
{
    {.cmd = PROTOCOLVERSION,    .handler = handle_protocol_version},
    {.cmd = INFO,               .handler = handle_info},
    {.cmd = READ,               .handler = handle_read},
    {.cmd = WRITE,              .handler = handle_write},
    {.cmd = BLOCKREAD,          .handler = handle_blockread},
    {.cmd = BLOCKWRITE_START,   .handler = handle_blockwrite_start},
    {.cmd = BLOCKWRITE_CONT,    .handler = handle_blockwrite_cont},
    {.cmd = BLOCKWRITE_END,     .handler = handle_blockwrite_end},
    {.cmd = DETECT,             .handler = handle_detect},
    {.cmd = DEVICE,             .handler = handle_unsupported},
    {.cmd = DRIVERCONTROL,      .handler = handle_unsupported},
    {.cmd = SERVICEMESSAGE,     .handler = handle_unsupported},
    {.cmd = SERVICEAVAILABLE,   .handler = handle_invalid},
    {.cmd = SHUTDOWN,           .handler = handle_unsupported},
    {.cmd = CURRENT_DEVICE,     .handler = handle_current_device}
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

// Return number of parts in str (split at ' ')
static uint8_t count_parts_in_cmd(char *cmd)
{
    uint8_t num = 1;
    size_t len;

    len = strlen(cmd);

    for (size_t i = 0; i < len; i++)
    {
        if (cmd[i] == ' ')
        {
            num++;
        }
    }

    return num;
}

/*
 * Functions that handle each of the bridge commands that we support
 */

static uint32_t handle_protocol_version(char *cmd)
{
    strcpy(cmd, "ProtocolVersion 106");

    return BRIDGE_STATUS_OK;
}

static uint32_t handle_info(char *cmd)
{
    // Abrreviated format: "app,versions,protocolversions,systemID,OS,OSversion"
    sprintf(cmd, "%s,%s,%s,%s,%s,%s", APP_NAME, APP_VER, PROTO_VER, "\"" SYSTEM_ID "\"", OP_SYS, OP_SYS_VER);

    return BRIDGE_STATUS_OK;
}

// User has executed a single register read command on WISCE/SCS.
static uint32_t handle_read(char *cmd)
{
    char cmd_action[3]={0}, cmd_chip_num[3]={0}, cmd_reg[9]={0};
    uint8_t num_parts = count_parts_in_cmd(cmd);
    char *end;
    uint32_t ret;
    uint32_t reg_val = 0;

    // Received msg format from Agent will be:
    // "RE [N] <HexRegAddr>\n"   N Id's the chip in a multi-chip device
    // Expect 2 parts if single chip device "RE <regAddr>\n"
    // 3 parts for double chip "RE <N> <regAddr>\n"
    if (num_parts == 2)
    {
        sscanf(cmd, "%s %s", cmd_action, cmd_reg);
    }
    else if (num_parts == 3)
    {
        sscanf(cmd, "%s %s %s", cmd_action, cmd_chip_num, cmd_reg);
        uint8_t device_index = (atoi(cmd_chip_num) - 1);
        if (device_index < bridge.num_devices)
        {
            bridge.current_device = &(bridge.device_list[device_index]);
        }
        else
        {
            sprintf(cmd, "%s", WMT_NO_DEVICE);
            return BRIDGE_STATUS_FAIL;
        }
    }
    else
    {
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }

    /* All references to functions beginning with regmap_ are calls to the underlying bus between
     * the MCU and the device over either I2C or SPI, depending on the implementation.
     * Replace these calls using the bus access API of your implementaion.
     */
    ret = regmap_read(&(bridge.current_device->b), strtoul(cmd_reg, &end, 10), &reg_val);
    if (ret != REGMAP_STATUS_OK)
    {
        sprintf(cmd, "%s", WMT_READ_FAILED);
        return BRIDGE_STATUS_FAIL;
    }
    sprintf(cmd, "%lu", reg_val);

    return BRIDGE_STATUS_OK;
}

// User has executed a single register write command on WISCE/SCS.
static uint32_t handle_write(char *cmd)
{
    char cmd_action[3]={0}, cmd_chip_num[3]={0}, cmd_reg[9]={0}, cmd_val[9]={0};
    uint8_t num_parts = count_parts_in_cmd(cmd);
    char *end;
    uint32_t ret;

    // Expect 3 parts if single chip device "WR <regAddr> <32bitval>"
    // 4 parts for double chip "WR <N> <regAddr> <32bitval>"
    if (num_parts == 3)
    {
        sscanf(cmd, "%s %s %s", cmd_action, cmd_reg, cmd_val);
    }
    else if (num_parts == 4)
    {
        sscanf(cmd, "%s %s %s %s", cmd_action, cmd_chip_num, cmd_reg, cmd_val);
        uint8_t device_index = (atoi(cmd_chip_num) - 1);
        if (device_index < bridge.num_devices)
        {
            bridge.current_device = &(bridge.device_list[device_index]);
        }
        else
        {
            sprintf(cmd, "%s", WMT_NO_DEVICE);
            return BRIDGE_STATUS_FAIL;
        }
    }
    else
    {
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }

    ret = regmap_write(&(bridge.current_device->b), strtoul(cmd_reg, &end, 10), strtoul(cmd_val, &end, 10));
    if (ret != REGMAP_STATUS_OK)
    {
        sprintf(cmd, "%s", GENERAL_FAILURE);
        return BRIDGE_STATUS_FAIL;
    }

    sprintf(cmd, "%s", WRITE_OK);

    return BRIDGE_STATUS_OK;
}

// User has executed a block-read command on WISCE/SCS.
static uint32_t handle_blockread(char *cmd)
{
    char cmd_action[3]={0}, cmd_chip_num[2]={0}, cmd_reg[9]={0};
    char cmd_len[8]={0};  // holds num bytes to read in a blockread.
    uint8_t num_parts = count_parts_in_cmd(cmd);
    char *end;
    uint32_t ret;
    uint32_t block_read_length, reg_addr;

    // Example cmd from Agent: "BR 0 8\n"  for single chip
    // Expect 3 parts if single chip device "BR <start> <len>"
    // 4 parts for double chip              "BR <N> <start> <len>"
    if (num_parts == 3)
    {
        sscanf(cmd, "%s %s %s", cmd_action, cmd_reg, cmd_len);
    }
    else if (num_parts == 4)
    {
        sscanf(cmd, "%s %s %s %s", cmd_action, cmd_chip_num, cmd_reg, cmd_len);
        uint8_t device_index = (atoi(cmd_chip_num) - 1);
        if (device_index < bridge.num_devices)
        {
            bridge.current_device = &(bridge.device_list[device_index]);
        }
        else
        {
            sprintf(cmd, "%s", WMT_NO_DEVICE);
            return BRIDGE_STATUS_FAIL;
        }
    }
    else
    {
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }

    // Impose a bytes limit of MAX_BLOCK_DATA_BYTES
    block_read_length = strtoul(cmd_len, &end, 10);
    reg_addr = strtoul(cmd_reg, &end, 10);

    if (block_read_length > MAX_BLOCK_DATA_BYTES)
    {
        sprintf(cmd, "%s", WMT_UNSUPPORTED);
        return BRIDGE_STATUS_FAIL;
    }

    ret = regmap_read_block(&(bridge.current_device->b), reg_addr, block_buffer, block_read_length);
    if(ret != REGMAP_STATUS_OK)
    {
        sprintf(cmd, "%s", WMT_READ_FAILED);
        return BRIDGE_STATUS_FAIL;
    }

    for(uint32_t i = 0; i < block_read_length; ++i)
    {
        // Convert each byte value into ASCII hex
        sprintf(&cmd[i*2], "%02X", block_buffer[i]);
    }
    cmd[block_read_length*2] = '\0'; // Manually add null terminator

    return BRIDGE_STATUS_OK;
}

// User has executed a block-write command on WISCE/SCS.
// The bridge agent breaks block-write commands into a series of messages, each carrying one word
// to be written to the hardware. Now we reassemble the messages into a single block write to be
// written to hardware.
static uint32_t handle_blockwrite_start(char *cmd)
{
    char cmd_action[3]={0}, cmd_chip_num[2]={0}, cmd_reg[9]={0};
    uint8_t num_parts = count_parts_in_cmd(cmd);
    char *end;
    char bw_single_reg_data_in[12] = {0};
    uint32_t data;

    // The normal cmd structure:
    // Client to Agent: "[<seq>]       BlockWrite <start> <data>\n"  Assuming we're not implementing for ADSP cores!!
    // Eg:              "[Shelley-1:9e] BW         2      00010203\n"
    // Agent to here:     "BW 2 00010203\n"  for single chip
    // Expect 3 parts if single chip device "BW <start> <data>"
    // 4 parts for double chip              "BW <N> <start> <data>"

    /* At Agent we have a long string where each 8 literals is the value
    for 1 reg. Eg  this is the value for 2 registers: "deadbeefdeadbeef"
    We could get the agent to pass this to here as is, but MCU would have to do a lot of
    work to a) separate the 8 ascii hex byte parcels and then convert hex parcel to dec.
    Instead do parceling at agent and pass each register's write value in decimal string to
    here in chunked sends over mUAURT. We must collect all the string parcels, convert to uint32_t values &
    store in an uint8_t array of size 200*4, since 200 register max wisce chunks, each reg holds 4 bytes
    Once we have all the data from agent we use block_write api
    */
    /* NEW CHUNKED CMD STRUCTURE
            MCU                                    Agent
                <-- "BWs [N] <addr> <XXXXXXXX>\n"
                    "BWc\n"  -->
                <-- "BWc <XXXXXXXX>\n"
                    "BWc\n"  -->
                    :
                <-- "BWe [<XXXXXXXX>]\n"

                    "Ok\n"  -->
                    or
                    "ER <N>\n"
    */
    if (num_parts == 3)
    {
        // Expect "BWs <addr> <XXXXXXXXXX>\n"   <XXXXXXXXXX> is dec ascii
        sscanf(cmd, "%s %s %s", cmd_action, cmd_reg, bw_single_reg_data_in);
    }
    else if (num_parts == 4)
    {
        // Expect "BWs N <addr> <XXXXXXXXXX>\n"
        sscanf(cmd, "%s %s %s %s", cmd_action, cmd_chip_num, cmd_reg, bw_single_reg_data_in);
        uint8_t device_index = (atoi(cmd_chip_num) - 1);
        if (device_index < bridge.num_devices)
        {
            bridge.current_device = &(bridge.device_list[device_index]);
        }
        else
        {
            sprintf(cmd, "%s", WMT_NO_DEVICE);
            return BRIDGE_STATUS_FAIL;
        }
    }
    else
    {
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }

    // Incoming reg addr and BW data will be in hex string form
    // Store to new blockwrite context for collecting chunked data
    bw_chip_num = strtoul(cmd_chip_num, &end, 10);
    bw_addr = strtoul(cmd_reg, &end, 10);
    bw_data_collect_indx = 0;
    data = strtoul(bw_single_reg_data_in, &end, 10);
    block_buffer[bw_data_collect_indx++] = (data >> 24) & 0xFF;
    block_buffer[bw_data_collect_indx++] = (data >> 16) & 0xFF;
    block_buffer[bw_data_collect_indx++] = (data >> 8) & 0xFF;
    block_buffer[bw_data_collect_indx++] = data & 0xFF;
    sprintf(cmd, "%s", BLOCKWRITE_CONT);

    return BRIDGE_STATUS_OK;
}

static uint32_t handle_blockwrite_cont(char *cmd)
{
    char cmd_action[3]={0};
    uint8_t num_parts = count_parts_in_cmd(cmd);
    char *end;
    char bw_single_reg_data_in[12] = {0};
    uint32_t data;

    /*
                <-- "BWc <XXXXXXXX>\n"
                    "BWc\n"  -->
                    or
                    "ER <N>\n"
    */
    if (num_parts != 2)
    {
       // Expect 2 parts in command
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }
    sscanf(cmd, "%s %s", cmd_action, bw_single_reg_data_in);
    if (bw_data_collect_indx >= BRIDGE_BLOCK_BUFFER_LENGTH_BYTES)
    {
        // Abort
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }
    data = strtoul(bw_single_reg_data_in, &end, 10);
    block_buffer[bw_data_collect_indx++] = (data >> 24) & 0xFF;
    block_buffer[bw_data_collect_indx++] = (data >> 16) & 0xFF;
    block_buffer[bw_data_collect_indx++] = (data >> 8) & 0xFF;
    block_buffer[bw_data_collect_indx++] = data & 0xFF;
    sprintf(cmd, "%s", BLOCKWRITE_CONT);

    return BRIDGE_STATUS_OK;
}

static uint32_t handle_blockwrite_end(char *cmd)
{
    char cmd_action[3]={0};
    uint8_t num_parts = count_parts_in_cmd(cmd);
    uint32_t ret;

    /*
                <-- "BWe\n"   (No data)
                    "Ok\n"  -->
                    or
                    "ER <N>\n"
    */
    if (num_parts == 1)
    {
        sscanf(cmd, "%s", cmd_action);
    }
    else
    {
       // This is really a bridge/us error, could be better handled b/w us and Agent
        sprintf(cmd, "%s", WMT_INVALID_COMMAND);
        return BRIDGE_STATUS_FAIL;
    }

    // Have all data, do block write
    ret = regmap_write_block(&(bridge.current_device->b), bw_addr , &block_buffer[0], bw_data_collect_indx);
    if(ret != REGMAP_STATUS_OK)
    {
        sprintf(cmd, "%s", GENERAL_FAILURE);
        return BRIDGE_STATUS_FAIL;
    }
    sprintf(cmd, "%s", WRITE_OK);

    return BRIDGE_STATUS_OK;
}

static uint32_t handle_detect(char *cmd)
{
    const char *bus_name_str;
    char temp_str[MAX_BRIDGE_DEVICE_NAME_LEN+1]; // Holds the dev_name_str eg "Shelley_Left,", plus one for safety

    cmd[0] = '\0';  // 0-out the cmd string for reuse
    for (uint8_t i = 0; i < bridge.num_devices; i++)
    {
        if (bridge.device_list[i].b.bus_type == REGMAP_BUS_TYPE_SPI)
        {
            bus_name_str = bus_name_spi;
        }
#ifdef CONFIG_USE_VREGMAP
        else if (bridge.device_list[i].b.bus_type == REGMAP_BUS_TYPE_I2C)
        {
            bus_name_str = bus_name_i2c;
        }
        else // only other option is REGMAP_BUS_TYPE_VIRTUAL
        {
            bus_name_str = bus_name_vregmap;
        }
#else
        else
        {
            bus_name_str = bus_name_i2c;
        }
#endif

        sprintf(temp_str, "%s,", bridge.device_list[i].dev_name_str);
        strcat(cmd, temp_str);
        sprintf(temp_str, "%s,", bus_name_str);
        strcat(cmd, temp_str);
        sprintf(temp_str, "%x,", bridge.device_list[i].bus_i2c_cs_address);
        strcat(cmd, temp_str);
        sprintf(temp_str, "%s,", DRIVER_CTRL);
        strcat(cmd, temp_str);
        sprintf(temp_str, "%s:", bridge.device_list[i].device_id_str);
        strcat(cmd, temp_str);
    }

    uint32_t index = strlen(cmd) - 1;
    cmd[index] = '\0'; // remove the final ':'

    return BRIDGE_STATUS_OK;
}

static uint32_t handle_unsupported(char *cmd)
{
    sprintf(cmd, "%s", WMT_UNSUPPORTED);

    return BRIDGE_STATUS_FAIL;
}

static uint32_t handle_invalid(char *cmd)
{
    sprintf(cmd, "%s", WMT_INVALID_COMMAND);

    return BRIDGE_STATUS_FAIL;
}

static uint32_t handle_current_device(char *cmd)
{
    sprintf(cmd, "%s", bridge.current_device->dev_name_str);

    return BRIDGE_STATUS_OK;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize the bridge processing module.
 *
 */
uint32_t bridge_initialize(bridge_device_t *device_list, uint8_t num_devices)
{
    if (device_list == NULL)
    {
        return BRIDGE_STATUS_FAIL;
    }

    bridge.device_list = device_list;
    bridge.num_devices = num_devices;
    bridge.current_device = device_list;

#ifdef CONFIG_USE_VREGMAP
    device_list[0].device_id_str = VREGMAP_BRIDGE_DEVICE_ID;
    device_list[0].dev_name_str = VREGMAP_BRIDGE_DEV_NAME;
    device_list[0].b = vregmap_cp;

    /*
     * A weird quirk of WISCE - it does not like devices of different types (I2C,SPI) in the same system.  So, if
     * there's another device in the list, vregmap bus type string must match.  default is I2C, so only check for SPI
     */
    if ((num_devices > 1) && (device_list[1].b.bus_type == REGMAP_BUS_TYPE_SPI))
    {
        bus_name_vregmap = bus_name_spi;
    }

    for (uint8_t i = 0; i < VREGMAP_LENGTH_REGS; i++)
    {
        vregmap[i].value = vregmap[i].default_value;
    }
#endif

    return BRIDGE_STATUS_OK;
}

/**
 * Wait for and process any incoming bridge commands from the transport between
 * the bridge agent (running on a host) and the MCU where this code is running.
 * This should be called in a continuous loop eg from the main function of the program.
 * It gathers characters until a '\n' character is received which signals the end of
 * a bridge command.
 */
void bridge_process(void)
{
    uint32_t val, i = 0;

    /* Try to read the command sent from the transport between bridge agent and MCU.
     * In this implementation we are using a multi-packet UART, but other implementions
     * should replace this code with appropriate calls to their transport API.
     * Ensure that a complete command is received before attempting to process it,
     * terminated by a '\n'.
     */
    do {
        /* In this implementation, fgetc overrides the std C system call and utilizes
         * a multi-packet UART. The semantics are the same as the std system call.
         * The function does not block
         */
        val = fgetc(bridge_read_file);
        if (val == EOF)
        {
            if (i)
            {
                // No more data, but we're mid transaction, so keep waiting
                continue;
            }
            else
            {
                // No data, so just return
                break;
            }
        }

        cmd_resp[i++] = (char) val;
    } while (val != '\n');

    // If we received a complete Cmd from Agent, process it
    if (i)
    {
        uint32_t ret;
        bridge_command_handler_t handler = NULL;
        cmd_resp[i++] = '\0';

        // Find the correct handler for the bridge command
        for (uint8_t i = 0; i < (sizeof(command_handler_map)/sizeof(bridge_command_handler_map_t)); i++)
        {
            if (strncmp(cmd_resp, command_handler_map[i].cmd, strlen(command_handler_map[i].cmd)) == 0)
            {
                handler = command_handler_map[i].handler;
            }
        }

        if (handler == NULL)
        {
            handler = handle_unsupported;
        }

        ret = handler(cmd_resp);

        if (ret != BRIDGE_STATUS_OK)
        {
            // Handler returned an error so send an error msg back to bridge
            fprintf(bridge_write_file, "%s %s\n", ERROR, cmd_resp);
        }
        else
        {
            /* Handler returned OK so send the response back to bridge.
             * Again, the fprintf overrides the C std system call and uses multi-packet UART
             * to transmit the data to the bridge agent.
             * For other transport, replace this call with the appropriate calls to
             * the transport API.
             */
            fprintf(bridge_write_file, "%s\n", cmd_resp);
        }
    }
}
