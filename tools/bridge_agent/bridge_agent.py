#!/usr/bin/python
#==========================================================================
# (c) 2021-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : StudioBridge Server to translate from TCP/IP to UART
# File    : bridge_agent.py
#--------------------------------------------------------------------------
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#--------------------------------------------------------------------------
#
# Environment Requirements: None
#
#==========================================================================
from __future__ import print_function
import sys
import socket
import os
import signal

# Bridge to Alt-OS MCU internal message protocol version
BRIDGE_MCU_MSG_FORMAT = "0.1"

CLIENT_PORT = 22349
SOCK_RX_BYTES = 2048

ser = None
SER_BAUD = 115200  # Can be increased
DEVICE_COM_PORT = 'COM8'  # Change according to your COM port to device

sock = None

# Shorthand of Commands sent to MCU. Needed as some commands can be specified
# by a client in abbreviated form
cmd_mcu_abbreviated = {
    "ProtocolVersion"   :"PV",
    "Info"              :"IN",
    "Read"              :"RE",
    "R"                 :"RE",
    "Write"             :"WR",
    "W"                 :"WR",
    "BlockRead"         :"BR",
    "BR"                :"BR",
    "BlockWrite"        :"BW",
    "BW"                :"BW",
    "Detect"            :"DT",
    "Device"            :"DV",
    "DriverControl"     :"DC",
    "ServiceMessage"    :"SM",
    "SM"                :"SM",
    "ServiceAvailable"  :"SA",
    "Shutdown"          :"SD",
    "IntBridgeMcuMsgVersion":"IV"
}

cmd_mcu_opcodes = {
    "CD"                    :0x1,
    "ProtocolVersion"       :0x2,
    "PV"                    :0x2,
    "Info"                  :0x3,
    "IN"                    :0x3,
    "Detect"                :0x4,
    "DT"                    :0x4,
    "Read"                  :0x5,
    "R"                     :0x5,
    "RE"                    :0x5,
    "Write"                 :0x6,
    "W"                     :0x6,
    "WR"                    :0x6,
    "BlockRead"             :0x7,
    "BR"                    :0x7,
    "BlockWriteStart"       :0x8,
    "BWs"                   :0x8,
    "BWc"                   :0x9,
    "BWe"                   :0xa,
    "DV"                    :0xb,   # Device
    "DC"                    :0xc,   # DriverControl
    "SM"                    :0xd,   # ServiceMessage
    "SA"                    :0xe,   # ServiceAvailable
    "SD"                    :0xf,   # Shutdown
    "IntBridgeMcuMsgVersion":0x10
}

cmds_with_numerical_args = ["R", "Read", "BlockRead", "BR", "W", "Write", "BlockWrite", "BW"]

no_arg_abbrv_cmds = ["CD", "IN", "DT", "DV", "DC", "SM", "SA", "SD"]

ERROR_REPLY = "ER"
DEFAULT_NUM_CHIPS = 1

BRIDGE_STATE_HANDSHAKE_GET_CD                   = 0
BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_GET_CD    = 1
BRIDGE_STATE_MCU_MSG_FORMAT_VERSION             = 2
BRIDGE_STATE_WAIT_MCU_MSG_FORMAT_VERSION        = 3
BRIDGE_STATE_HANDSHAKE_INFO                     = 4
BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_INFO      = 5
BRIDGE_STATE_WAIT_CLI_CMD                       = 6
BRIDGE_STATE_WAIT_MCU_REPLY                     = 7
# States for executing a block-write operation
BRIDGE_STATE_BW_START                           = 11
BRIDGE_STATE_BW_CONTINUE                        = 12
BRIDGE_STATE_BW_END                             = 13
BRIDGE_STATE_BW_DONE                            = 14
BWs = "BWs"
BWc = "BWc"
BWe = "BWe"

# This is the device Id WISCE wants to be given as part of the Detect command,
# which WISCE uses to load the correct device pack.
wisce_device_id = "unknown"

devices = dict()  # No device details discovered yet
num_chips = DEFAULT_NUM_CHIPS
verbose = False

PAYLOAD_BYTE_LENGTH = 2
# Define how binary data should be sent in payload of smcio packets
# All integral values in the payload are sent in little-endian format to the MCU as
# the Alt-OS development board uses tools that are compiled as little-endian
PAYLOAD_BINARY_ENDIANNESS = 'little'
# These are used by smcio.py. Uncomment according to how PAYLOAD_BINARY_ENDIANNESS has been defined
#PAYLOAD_UNPACK_SHORT = ">H"   # Big endian unsigned short
#PAYLOAD_UNPACK_INT   = ">I"   # Big endian unsigned int
PAYLOAD_UNPACK_SHORT = "<H"   # Little endian unsigned short
PAYLOAD_UNPACK_INT   = "<I"   # Little endian unsigned int


# Translation table to go from <name> string from Detect reply to an integer
class Name_To_Int_Id(object):
    def __init__(self):
        super().__init__()
        self.name_to_int_id = dict()

    def clear(self):
        self.name_to_int_id.clear()

    def name_exists(self, name):
        for k,v in self.name_to_int_id.items():
            if name == k:
                return True
        return False

    def add_new_name(self, name, id):
        if self.name_exists(name):
            print("***Warning***\ndevice name {} is duplicate of existing name".format(name))
            print("Behaviour is not defined")
        self.name_to_int_id[name] = id

    def get_id(self, name):
        return self.name_to_int_id[name]

    def print_all(self):
        print("device name to Id translation (used in MCU cmds): {}".format(self.name_to_int_id))

name_to_int_id = Name_To_Int_Id()

class bridge_excpn(Exception):
    pass

class bridge_sock_excpn(Exception):
    pass

class blockwrite_chunk_excpn(Exception):
    pass

class missing_chip_id_excpn(Exception):
    pass

def init_bridge_socket(host='127.0.0.1', port=CLIENT_PORT):
    global sock

    print('Creating Socket to host: {}, port: {}'.format(host, port))

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind((host, port))
    except socket.error as msg:
        try:
            sock.bind((host, 0)) # if default port is taken, get one auto assigned
        except socket.error as msg:
            print('Socket Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
            sys.exit()

    print('Socket bind complete')
    # Set socket's NoDelay flag
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    sock.listen(10)
    portstr = format(sock.getsockname()[1], 'd')
    print("Socket listening on port: {}\n".format(portstr))

    sock.settimeout(1)
    conn = None
    addr = None
    while (conn == None and addr == None):
        try:
            conn, addr = sock.accept() # repeat accept until success, will react to Ctrl+C every 1 sec
        except OSError:
            pass

    print("Socket connected\n")
    return (conn, addr)

# ToDo Use with open for socket and serial if you can
def close_bridge(sock, bridgecli_sockcon):
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

def close_serial(ser):
    if ser.is_open:
        ser.close()

def check_msg(msg):
    if msg:
        i = 1
        while ord(msg[-i]) == 10:
            i += 1
        print("{} has {} new line chars".format(msg, i-1))
        return i-1
    else:
        print("ERROR str passed to check_msg() is empty!")


# TODO: Should use logging module instead
def dbg_pr_general(verbose, msg):
    if verbose:
        print(msg)

def dbg_pr_ClientMsg(verbose, msg):
    if verbose:
        print("**** NEW WISCE MSG ****\n\t\t\t{}\t\t <- Client Command".format(msg))
        check_msg(msg)

def dbg_pr_AgentMsgToDevice(verbose, msg):
    if verbose:
        print("Device \t <- {} Agent relay Command".format(msg))
        check_msg(msg)

def dbg_pr_DeviceMsgToAgent(verbose, msg):
    if verbose:
        print("Device reply: {} -> Agent".format(msg))
        check_msg(msg)

def dbg_pr_AgentMsgToClient(verbose, msg):
    if verbose:
        print("Agent reply: {} -> Client".format(msg))
        check_msg(msg)

def payload_length_bytes(payload_len):
    return payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)

#==========================================================================
# Current Command object
#=========================================================================
class current_command(object):
    recvd_cmd_b = b''   # ToDo could make these instance vars
    seq_num = None
    device_name = ""
    action = ""
    arg1 = None
    arg2 = None

    def get_all_bytes(self):
        return self.__class__.recvd_cmd_b

    def get_all_str(self):
        return str(self.__class__.recvd_cmd_b, 'UTF-8')

    def get_action_str(self):
        return self.__class__.action

    def new_cmd(self, cmd_b):
        self.__class__.recvd_cmd_b = cmd_b
        '''Incoming cmd from client can be of the form
        1. "[<deviceName>:<SeqNum>] Read <reg>" or "[<deviceName>:<SeqNum>] BlockRead <starReg> <numBytes>"
        2. "[<deviceName>:<SeqNum>] Write <reg> <val>" or "[<deviceName>:<SeqNum>] BlockWrite <StartReg> <data>"
        3. "[<deviceName>:<SeqNum>] R <reg>" or "[<deviceName>:<SeqNum>] BR <starReg> <numBytes>"
        4. "[<deviceName>:<SeqNum>] W <reg> <val>" or "[<deviceName>:<SeqNum>] BW <StartReg> <data>"
        5. "Read <reg>" or "BlockRead <starReg> <numBytes>"
        6. "Write <reg> <val>" or "BlockWrite <StartReg> <data>"
        7. "R <reg>" or "BR <starReg> <numBytes>"
        8. "W <reg> <val>" or "BW <StartReg> <data>"
        9. "Info"
        10."ProtocolVersion 105"
        11. "[SeqNum] <command>"
        '''
        self.__class__.seq_num = None
        parts = str(cmd_b, 'UTF-8').split()
        if '[' in parts[0]:
            # Cmd has seq num part
            part1 = parts[0]
            if ':' in part1:
                # Eg "[CS47L63-1:2] R c08"
                self.__class__.device_name = part1[1: part1.find(':')]
                self.__class__.seq_num = part1[part1.find(':') + 1: part1.find(']')]
            else:
                # Eg "[4] Detect" or "[4]  Read <reg>"
                self.__class__.device_name = None
                self.__class__.seq_num = part1[part1.find('[') + 1: part1.find(']')]
            parts.pop(0)
        self.__class__.action = parts[0]
        if len(parts) == 1:
            self.__class__.arg1 = None
            self.__class__.arg2 = None
        elif len(parts) == 2:
            self.__class__.arg1 = parts[1]
            self.__class__.arg2 = None
            if self.__class__.action in cmds_with_numerical_args:
                self.__class__.arg1 = str(int(self.__class__.arg1, 16))
        elif len(parts) == 3:
            self.__class__.arg1 = parts[1]
            self.__class__.arg2 = parts[2]
            if self.__class__.action in cmds_with_numerical_args:
                self.__class__.arg1 = str(int(self.__class__.arg1, 16))
                self.__class__.arg2 = str(int(self.__class__.arg2, 16))
        else:
            raise bridge_excpn("Unexpected Cmd format received: {}".format(cmd_b))
        if self.__class__.action not in cmd_mcu_abbreviated:
            raise bridge_excpn("Unexpected Cmd action received: {}".format(cmd_b))


def hexstr_to_decstr(hexstr):
    if hexstr is not None and len(hexstr):
        return str(int(hexstr, 16))
    else:
        return None

#==========================================================================
# Send block-write cmd data to device in chunks
#=========================================================================
def send_bw_data_to_mcu_binary(crnt_cmd, chip_id, ser_ch, ch_num, state, verbose, user_num_reg_in_chunk):
    if crnt_cmd.arg1 is None or crnt_cmd.arg2 is None:
        raise blockwrite_chunk_excpn("BlockWrite cmd has no addr and/or data")

    cmd_str = crnt_cmd.get_all_str()
    data_str = cmd_str.split()[-1]
    if len(data_str) % 8 != 0:
        raise blockwrite_chunk_excpn("BlockWrite cmd data is invalid")

    data_indx = 0
    state = BRIDGE_STATE_BW_START
    try:
        while state != BRIDGE_STATE_BW_DONE:
            if state == BRIDGE_STATE_BW_START:
                dbg_pr_general(verbose, "Agent state BWs: write cmd to device")
                # Send MCU BW start msg
                # Calculate amount of registers to send based on what's left
                # and the max chunk size
                num_regs_left_to_send = int((len(data_str) - data_indx) / 8)
                if num_regs_left_to_send < user_num_reg_in_chunk:
                    reg_chunk_to_send = num_regs_left_to_send
                else:
                    reg_chunk_to_send = user_num_reg_in_chunk

                datachunk_str_hex = data_str[data_indx:8*reg_chunk_to_send]
                datachunk_int = int(datachunk_str_hex, 16)
                if chip_id is None:
                    ## ToDO Is this an error now? We expect a chip is for all cmds?
                    raise missing_chip_id_excpn("BW start chip Id is None")

                bin_payload = bytearray()
                chunk_bytes = reg_chunk_to_send * 4  # Assumes 4 bytes per register
                payload_len = 8 + chunk_bytes
                bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
                # Add OpCode
                bin_payload.append(cmd_mcu_opcodes["BWs"])
                # BW has ChipId field
                bin_payload.append(chip_id)   ## TODO: Can remove chip_id passed in param? Use crnt_cmd
                # Add 4-byte address to write
                write_addr_int = int(crnt_cmd.arg1)
                bin_payload += write_addr_int.to_bytes(4, PAYLOAD_BINARY_ENDIANNESS)
                # Add write value
                # NB: The MCU regmap block-write fn needs an array of byte data in big endian. The MCU
                # uses memcpy to grab the bytes so the data will end up keeping the big endian format
                bin_payload += datachunk_int.to_bytes(chunk_bytes, 'big')
                dbg_pr_AgentMsgToDevice(verbose, datachunk_str_hex)
                ser_ch.write_channel_bytes(ch_num, bin_payload)
                data_indx += 8 * reg_chunk_to_send
                if data_indx < len(data_str):
                    state = BRIDGE_STATE_BW_CONTINUE
                else:
                    state = BRIDGE_STATE_BW_END
            elif state == BRIDGE_STATE_BW_CONTINUE:
                dbg_pr_general(verbose, "Agent state BWc: Waiting for reply from device")
                mcu_reply_str = wait_for_serial_data(ser_ch, ch_num)
                # Check for MCU BWc or Error reply
                if len(mcu_reply_str) and BWc in mcu_reply_str[:len(BWc)]:
                    # Send next parcel of data
                    # Calculate amount of registers to send based on what's left
                    # and the max chunk size
                    num_regs_left_to_send = int((len(data_str)-data_indx)/8)
                    if num_regs_left_to_send < user_num_reg_in_chunk:
                        reg_chunk_to_send = num_regs_left_to_send
                    else:
                        reg_chunk_to_send = user_num_reg_in_chunk
                    datachunk_str_hex = data_str[data_indx:data_indx+(8*reg_chunk_to_send)]
                    datachunk_int = int(datachunk_str_hex, 16)
                    chunk_bytes = reg_chunk_to_send * 4  # Assumes 4 bytes per register
                    payload_len = 3 + chunk_bytes
                    bin_payload = bytearray()
                    bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
                    # Add OpCode
                    bin_payload.append(cmd_mcu_opcodes["BWc"])
                    # Add write value
                    # NB: The MCU regmap block-write fn needs an array of byte data in big endian. The MCU
                    # uses memcpy to grab the bytes so the data will end up keeping the big endian format
                    bin_payload += datachunk_int.to_bytes(chunk_bytes, 'big')
                    dbg_pr_AgentMsgToDevice(verbose, datachunk_str_hex)
                    ser_ch.write_channel_bytes(ch_num, bin_payload)
                    data_indx += 8 * reg_chunk_to_send
                    if data_indx < len(data_str):
                        state = BRIDGE_STATE_BW_CONTINUE
                    else:
                        state = BRIDGE_STATE_BW_END
                else:
                    # Ignoring error code sent by device for now
                    raise blockwrite_chunk_excpn("BWc: Device replied with some error")
            elif state == BRIDGE_STATE_BW_END:
                dbg_pr_general(verbose, "Agent state BWe: Waiting for last BWc reply from device")
                mcu_reply_str = wait_for_serial_data(ser_ch, ch_num)
                # Check for MCU BWc or Error reply
                if len(mcu_reply_str) and BWc in mcu_reply_str[:len(BWc)]:
                    dbg_pr_general(verbose, "Agent state BWe, write BWe to MCU")
                    payload_len = 3
                    bin_payload = bytearray()
                    bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
                    # Add OpCode
                    bin_payload.append(cmd_mcu_opcodes["BWe"])
                    ser_ch.write_channel_bytes(ch_num, bin_payload)
                else:
                    # Ignoring error code sent by device for now
                    raise blockwrite_chunk_excpn("BWc: Device replied with some error")
                state = BRIDGE_STATE_BW_DONE
    except Exception as be:
        print(be)
        raise blockwrite_chunk_excpn("Unknown error during Block Write operation between Agent and Device")


#==========================================================================
# Command Handler Function
#=========================================================================

# Creates binary fields according to cmd type and writes to serial channel
def client_cmd_handler_binary(crnt_cmd, ser_ch, ch_num, state, verbose, user_num_reg_in_chunk):
    # Switch on cmd type
    abbr_action_str = cmd_mcu_abbreviated[crnt_cmd.get_action_str()]
    if abbr_action_str == "BW":
        try:
            send_bw_data_to_mcu_binary(crnt_cmd, devices[crnt_cmd.device_name]["chip_id"],
                                       ser_ch, ch_num, state, verbose, user_num_reg_in_chunk)
        except (blockwrite_chunk_excpn, missing_chip_id_excpn) as bwe:
            print(bwe)
            raise
    elif abbr_action_str in no_arg_abbrv_cmds:
        bin_payload = bytearray()
        payload_len = 3
        # Add payload-length field (2 bytes)
        bin_payload += payload_length_bytes(payload_len)
        # Add OpCode
        bin_payload.append(cmd_mcu_opcodes[abbr_action_str])
        # Write binary payload to serial channel
        ser_ch.write_channel_bytes(ch_num, bin_payload)
    elif abbr_action_str == "PV":
        bin_payload = bytearray()
        payload_len = 5
        # Add payload-length field (2 bytes)
        bin_payload += payload_length_bytes(payload_len)
        # Add OpCode
        bin_payload.append(cmd_mcu_opcodes[abbr_action_str])
        # Add version from cmd's first arg. This will be eg "106"
        pv_int = int(crnt_cmd.arg1)
        bin_payload += pv_int.to_bytes(2, PAYLOAD_BINARY_ENDIANNESS)
        # Write binary payload to serial channel
        ser_ch.write_channel_bytes(ch_num, bin_payload)
    elif abbr_action_str == "RE":
        bin_payload = bytearray()
        payload_len = 8
        # Add payload-length field (2 bytes)
        bin_payload += payload_length_bytes(payload_len)
        # Add OpCode
        bin_payload.append(cmd_mcu_opcodes[abbr_action_str])
        # RE has ChipId field
        bin_payload.append(devices[crnt_cmd.device_name]["chip_id"])
        # Add 4-byte address to read
        read_addr_int = int(crnt_cmd.arg1)
        bin_payload += read_addr_int.to_bytes(4, PAYLOAD_BINARY_ENDIANNESS)
        # Write binary payload to serial channel
        ser_ch.write_channel_bytes(ch_num, bin_payload)
    elif abbr_action_str == "WR":
        bin_payload = bytearray()
        payload_len = 12
        # Add payload-length field (2 bytes)
        bin_payload += payload_length_bytes(payload_len)
        # Add OpCode
        bin_payload.append(cmd_mcu_opcodes[abbr_action_str])
        # WR has ChipId field
        bin_payload.append(devices[crnt_cmd.device_name]["chip_id"])
        # Add 4-byte address to write
        write_addr_int = int(crnt_cmd.arg1)
        bin_payload += write_addr_int.to_bytes(4, PAYLOAD_BINARY_ENDIANNESS)
        # Add write value
        write_val = int(crnt_cmd.arg2)
        bin_payload += write_val.to_bytes(4, PAYLOAD_BINARY_ENDIANNESS)
        # Write binary payload to serial channel
        ser_ch.write_channel_bytes(ch_num, bin_payload)
    elif abbr_action_str == "BR":
        bin_payload = bytearray()
        payload_len = 10
        # Add payload-length field (2 bytes)
        bin_payload += payload_length_bytes(payload_len)
        # Add OpCode
        bin_payload.append(cmd_mcu_opcodes[abbr_action_str])
        # BR has ChipId field
        bin_payload.append(devices[crnt_cmd.device_name]["chip_id"])
        # Add 4-byte address to read
        read_addr_int = int(crnt_cmd.arg1)
        bin_payload += read_addr_int.to_bytes(4, PAYLOAD_BINARY_ENDIANNESS)
        # Add number of bytes to read (2 bytes)
        read_len = int(crnt_cmd.arg2)
        bin_payload += read_len.to_bytes(2, PAYLOAD_BINARY_ENDIANNESS)
        # Write binary payload to serial channel
        ser_ch.write_channel_bytes(ch_num, bin_payload)
    else:
        raise Exception("Unknown abbr_action_str: {}".format(abbr_action_str))


def new_send_internal_CD_binary(ser_ch, ch_num):
    bin_payload = bytearray()
    payload_len = 3
    bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
    # Add OpCode
    bin_payload.append(cmd_mcu_opcodes["CD"])
    ser_ch.write_channel_bytes(ch_num, bin_payload)

def send_internal_bridge_mcu_protocol_version(ser_ch, ch_num):
    bin_payload = bytearray()
    payload_len = 3
    bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
    # Add OpCode
    bin_payload.append(cmd_mcu_opcodes["IntBridgeMcuMsgVersion"])
    ser_ch.write_channel_bytes(ch_num, bin_payload)

def send_internal_IN_binary(ser_ch, ch_num):
    bin_payload = bytearray()
    payload_len = 3
    bin_payload += payload_len.to_bytes(PAYLOAD_BYTE_LENGTH, PAYLOAD_BINARY_ENDIANNESS)
    # Add OpCode
    bin_payload.append(cmd_mcu_opcodes["IN"])
    ser_ch.write_channel_bytes(ch_num, bin_payload)

#==========================================================================
# MCU Reply handler Utility functions
# These process the various abbreviated replies from MCU and create correct
# reply to be sent back to the client
#=========================================================================
def mcu_reply_hndlr_info(mcu_reply_str_parts, handshake=False):
    # This may be called by the client -or- ourselves as part of initial handshake
    # For handshake purposes we extract
    # Expected MCU reply: "device,app,version,protocolversions,systemID,OS,OSversion"
    app, version, protver, systemid, _os, osversion = tuple(mcu_reply_str_parts)
    if(handshake):
        return wisce_device_id, app, version, protver, systemid, _os, osversion
    else:
        return \
            "device=\"{}\" app={} version={} protocolversion={} systemID={} os={} osversion={}\n".format(
                wisce_device_id, app, version, protver, systemid, _os, osversion)

def mcu_reply_hndlr_detect(mcu_detect_reply_s):
    '''As well as forming correct reply for the client, extract the device info for ourselves
    to discover what chips are present on the MCU.
    '''
    global num_chips, devices, name_to_int_id

    name_to_int_id.clear()

    # store info for ourselves
    num_chips = mcu_detect_reply_s.count(":") + 1
    mcu_reply_str_parts = mcu_detect_reply_s.split(':')
    reply_str = 'Detect '
    for i in range(num_chips):
        (name_n, bus_name, bus_addr, drvr_ctrl, id_str) = tuple(mcu_reply_str_parts[i].split(','))
        # Update the name_to_int_id translation to get the number to be used in future commands sent to MCU
        name_to_int_id.add_new_name(name_n, i + 1)

        devices[name_n] = dict()
        devices[name_n]["name"] = name_n
        devices[name_n]["chip_id"] = name_to_int_id.get_id(name_n)
        devices[name_n]["name_n"] = name_n
        devices[name_n]["bus"] = bus_name
        devices[name_n]["bus_addr"] = bus_addr
        devices[name_n]["drvr_ctrl"] = drvr_ctrl
        devices[name_n]["id"] = id_str

        reply_str += '{},{},{},DriverControl:{}="{}" '.format(
            name_n,
            devices[name_n]["bus"],
            devices[name_n]["bus_addr"],
            devices[name_n]["drvr_ctrl"],
            devices[name_n]["id"])

    # Print translation table
    name_to_int_id.print_all()

    # Formulate reply to client
    return (reply_str[:-1] + '\n')

def prepend_seq_num(current_cmd, cli_rsp_str):
    if current_cmd.seq_num is not None:
        cli_rsp_str = "[{}] {}".format(current_cmd.seq_num, cli_rsp_str)
    return cli_rsp_str

#==========================================================================
# Reply Handler Function
#=========================================================================
def do_reply_handler(current_cmd, mcu_reply_str):
    cli_rsp_str = None
    if current_cmd.action == "Detect":
        cli_rsp_str = mcu_reply_hndlr_detect(mcu_reply_str)
    elif current_cmd.action == "ProtocolVersion":
        cli_rsp_str = mcu_reply_str + "\n"

    elif current_cmd.action == "R" or current_cmd.action == "Read":
        # Expect MCU reply to be a reg read value "<regreadvalue>" in decimal ascii
        # So we need to Convert MCU's reg read value to hex for client
        mcu_reply_str_hex = hex(int(mcu_reply_str))
        # response to client format : "[seqNum] <regreadvalue>"
        cli_rsp_str = mcu_reply_str_hex + "\n"
    elif current_cmd.action == "W" or current_cmd.action == "Write" \
            or current_cmd.action == "BlockWrite" or current_cmd.action == "BW":
        # Expect MCU reply to be "Ok"
        # response to client format : "[seqNum] Ok"
        cli_rsp_str = "Ok\n"
    elif current_cmd.action == "BR" or current_cmd.action == "BlockRead":
        # Expect MCU reply to be string of multiple reg read values eg "0048ac40000000a0" No! will ne in decimal?
        # response to client format : "[seqNum] <regreadvalue>"
        cli_rsp_str = mcu_reply_str + "\n"

    elif current_cmd.action == "Info":
        cli_rsp_str = mcu_reply_hndlr_info(mcu_reply_str.split(','))

    else:
        print("NEED TO WRITE reply hndlr for Cmd {}, rxed reply: {}".format(current_cmd.action,
                                                                                mcu_reply_str))

    # Prepend seq num if valid, and return the client response string
    return prepend_seq_num(current_cmd, cli_rsp_str)


# First stage reply handlr to check for Error being sent from MCU
def reply_handler(current_cmd, mcu_reply_str):
    if ERROR_REPLY in mcu_reply_str[:len(ERROR_REPLY)]:
        cli_rsp_str = ''
        if current_cmd.seq_num is not None:
            cli_rsp_str = "[{}] ".format(current_cmd.seq_num)
        return "{}Error {}\n".format(cli_rsp_str, mcu_reply_str.split()[1])
    else:
        return do_reply_handler(current_cmd, mcu_reply_str)

def socket_send(sock, data_bytes):
    try:
        ret = sock.send(data_bytes)
        if ret == 0:
            raise bridge_excpn("Socket could not send data\n")
    except OSError as excp:
        print("Socket related error: {}".format(excp))
        raise excp

def wait_for_sock_recv(sock):
    try:
        # Receive Client Command
        sock.settimeout(1)
        recvd_data = 0
        flag = False
        while not flag:
            try:
                recvd_data = sock.recv(SOCK_RX_BYTES)  # repeat receive until no exception, will react to Ctrl+C every 1 sec
                flag = True
            except OSError:
                pass
        return recvd_data
    except OSError as excp:
        print("Socket related error: {}".format(excp))
        raise excp

def wait_for_serial_data(ser_ch, ch_num):
    # smcio read is non-blocking so loop read until we read a full string
    data_str = ""
    while '\n' not in data_str:
        data_tmp = ''
        while data_tmp == '':
            data_tmp = ser_ch.read_channel(ch_num)
        data_str = data_str + data_tmp
    return data_str


def inner_loop(sock, ser_ch, ch_num, state, crnt_cmd, verbose, user_num_reg_in_chunk):
    global wisce_device_id
    while True:
        try:
            dbg_pr_general(verbose, "Loop state: {}".format(state))
            if state == BRIDGE_STATE_HANDSHAKE_GET_CD:
                # Initiate a Current Device "CD" command
                new_send_internal_CD_binary(ser_ch, ch_num)
                state = BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_GET_CD
            elif state == BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_GET_CD:
                dbg_pr_general(verbose, "Initial read reg 0: Waiting for device reply")
                reply = wait_for_serial_data(ser_ch, ch_num)
                dbg_pr_DeviceMsgToAgent(verbose, reply)
                wisce_device_id = reply[:-1]
                print("wisce_device_id is {}".format(wisce_device_id))
                state = BRIDGE_STATE_MCU_MSG_FORMAT_VERSION
            elif state == BRIDGE_STATE_MCU_MSG_FORMAT_VERSION:
                send_internal_bridge_mcu_protocol_version(ser_ch, ch_num)
                state = BRIDGE_STATE_WAIT_MCU_MSG_FORMAT_VERSION
            elif state == BRIDGE_STATE_WAIT_MCU_MSG_FORMAT_VERSION:
                dbg_pr_general(verbose, "Internal Bridge to MCU Msg Format: Waiting for device reply")
                reply = wait_for_serial_data(ser_ch, ch_num)
                dbg_pr_DeviceMsgToAgent(verbose, reply)
                bridge_mcu_msg_format = reply[:-1]
                if bridge_mcu_msg_format != BRIDGE_MCU_MSG_FORMAT:
                    print("\n*WARNING* Bridge and MCU internal message formats are different")
                    print("Bridge message version: {}. MCU reports message version: {}".format(
                        BRIDGE_MCU_MSG_FORMAT, bridge_mcu_msg_format))
                    print("Some commands may not work as expected")
                else:
                    print("Bridge and MCU msg format versions match: {}".format(bridge_mcu_msg_format))
                state = BRIDGE_STATE_HANDSHAKE_INFO
            elif state == BRIDGE_STATE_HANDSHAKE_INFO:
                # Create abbr Info cmd
                # Send MCU INFO abbr cmd
                send_internal_IN_binary(ser_ch, ch_num)
                state = BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_INFO
            elif state == BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_INFO:
                dbg_pr_general(verbose, "INFO cmd handshake: Waiting for device reply")
                reply = wait_for_serial_data(ser_ch, ch_num)
                dbg_pr_DeviceMsgToAgent(verbose, reply)
                reply = reply[:-1]
                dev, app, version, protver, systemid, _os, osversion = \
                    mcu_reply_hndlr_info(reply.split(','), True)
                # Create handshake str for Cli
                agent_resp = "StudioBridge UART Version {}\n(c) Cirrus Logic\n{}\n".format(protver, dev)
                dbg_pr_AgentMsgToClient(verbose, agent_resp)
                socket_send(sock, agent_resp.encode())
                state = BRIDGE_STATE_WAIT_CLI_CMD
            elif state == BRIDGE_STATE_WAIT_CLI_CMD:
                dbg_pr_general(verbose, "Waiting for Client Command")
                cli_cmd_b = wait_for_sock_recv(sock)
                if not cli_cmd_b:
                    raise bridge_sock_excpn("No command received. Remote end may have terminated connection")
                crnt_cmd.new_cmd(cli_cmd_b)
                dbg_pr_ClientMsg(verbose, crnt_cmd.get_all_str())
                client_cmd_handler_binary(crnt_cmd, ser_ch, ch_num, state, verbose, user_num_reg_in_chunk)
                state = BRIDGE_STATE_WAIT_MCU_REPLY
            elif state == BRIDGE_STATE_WAIT_MCU_REPLY:
                dbg_pr_general(verbose, "Waiting for reply from device")
                mcu_reply = wait_for_serial_data(ser_ch, ch_num)
                dbg_pr_DeviceMsgToAgent(verbose, mcu_reply)
                mcu_reply = mcu_reply[:-1]
                client_resp_s = reply_handler(crnt_cmd, mcu_reply)
                dbg_pr_AgentMsgToClient(verbose, client_resp_s)
                socket_send(sock, client_resp_s.encode())
                state = BRIDGE_STATE_WAIT_CLI_CMD
            else:
                print("REACHED INCORRECT STATE. TERMINATING")
                sys.exit(1)

        except KeyboardInterrupt as kbe:
            os.kill(os.getpid(), signal.SIGINT)
        except bridge_sock_excpn as bse:
            print("{}\n".format(bse))
            raise OSError
        ## TODO: send_bw_data_to_mcu() can raise exception!
        ## TODO: Should catch general Exception here, send ER msg to client & continue loop

def outer_loop(ser_ch, ch_num, verbose, user_num_reg_in_chunk):
    while True:
        try:
            # Create socket & wait for connection to bridge client
            (bridgecli_sockcon, _) = init_bridge_socket()  # Blocks on socket conn
            state = BRIDGE_STATE_HANDSHAKE_GET_CD
            current_cmd = current_command()

            with bridgecli_sockcon:
                inner_loop(bridgecli_sockcon, ser_ch, ch_num, state, current_cmd, verbose, user_num_reg_in_chunk)

        except (TypeError, UnicodeError) as err:
            # Sometimes a client will send rubbish data down the socket
            # when a connection is closed, causing us to process garbage string.
            # Could be serious enough, start again with new connection
            print(type(err), err)
            print("Re-starting. Please attempt new WISCE/SCS connection")
        except KeyboardInterrupt as kbe:
            os.kill(os.getpid(), signal.SIGINT)
        except OSError as e:
            print("Socket related error: {}\nAny current operation will be aborted".format(e))


