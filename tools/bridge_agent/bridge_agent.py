#!/usr/bin/python
#==========================================================================
# (c) 2021 Cirrus Logic, Inc.
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
import time
import socket
import argparse

CLIENT_PORT = 22349
SOCK_RX_BYTES = 2048

ser = None
SER_BAUD = 115200  # Can be increased
DEVICE_COM_PORT = 'COM8'  # Change according to your COM port to device

sock = None

# Shorthand of Commands sent to MCU
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
}

cmds_with_numerical_args = ["R", "Read", "BlockRead", "BR", "W", "Write", "BlockWrite", "BW"]

ERROR_REPLY = "ER"
DEFAULT_NUM_CHIPS = 1

BRIDGE_STATE_HANDSHAKE_READ_REG0                = 0
BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_READ_REG0 = 1
BRIDGE_STATE_HANDSHAKE_INFO                     = 2
BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_INFO      = 3
BRIDGE_STATE_WAIT_CLI_CMD                       = 4
BRIDGE_STATE_WAIT_MCU_REPLY                     = 5
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

class bridge_excpn(Exception):
    pass

class bridge_sock_excpn(Exception):
    pass

class blockwrite_chunk_excpn(Exception):
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

    sock.listen(10)
    portstr = format(sock.getsockname()[1], 'd')
    print("Socket listening on port: {}\n".format(portstr))

    conn, addr = sock.accept()
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

def send_bw_data_to_mcu(crnt_cmd, ser_ch, ch_num, state, verbose):
    if crnt_cmd.arg1 is None or crnt_cmd.arg2 is None:
        raise blockwrite_chunk_excpn("BlockWrite cmd has no addr and/or data")
    # TODO we should remove BW from cmds_with_numerical_args then can still use arg2
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
                # Send MCU BW start msg: "BWs <regAddr> <regData>\n"
                # TODO handle chip num for multi chip platforms
                parcel_str_hex = data_str[data_indx:8]
                parcel_str_dec = hexstr_to_decstr(parcel_str_hex)
                dev_cmd_str = "{} {} {}\n".format(BWs, crnt_cmd.arg1, parcel_str_dec)
                dbg_pr_AgentMsgToDevice(verbose, dev_cmd_str)
                ser_ch.write_channel(ch_num, dev_cmd_str)
                data_indx += 8
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
                    parcel_str_hex = data_str[data_indx:data_indx+8]
                    parcel_str_dec = hexstr_to_decstr(parcel_str_hex)
                    dev_cmd_str = "{} {}\n".format(BWc, parcel_str_dec)
                    dbg_pr_AgentMsgToDevice(verbose, dev_cmd_str)
                    ser_ch.write_channel(ch_num, dev_cmd_str)
                    data_indx += 8
                    if data_indx < len(data_str):
                        state = BRIDGE_STATE_BW_CONTINUE
                    else:
                        state = BRIDGE_STATE_BW_END
                else:
                    # Ignoring error code sent by device for now
                    raise blockwrite_chunk_excpn("BWc: Device replied with some error")
            elif state == BRIDGE_STATE_BW_END:
                dbg_pr_general(verbose, "Agent state BWe: Waiting for last BWc reply from device")
                time.sleep(1)   # This was necessary to throttle
                mcu_reply_str = wait_for_serial_data(ser_ch, ch_num)
                # Check for MCU BWc or Error reply
                if len(mcu_reply_str) and BWc in mcu_reply_str[:len(BWc)]:
                    dbg_pr_general(verbose, "Agent state BWe, write BWe to MCU")
                    ser_ch.write_channel(ch_num, "{}\n".format(BWe))
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
def client_cmd_handler(crnt_cmd, ser_ch, ch_num, state, verbose):
    abbr_action_str = cmd_mcu_abbreviated[crnt_cmd.get_action_str()]
    abbr_dev_cmd_str = abbr_action_str

    # add chip target if multi-chip
    if ((num_chips > 1) and (crnt_cmd.device_name is not None)):
        # Expect the client cmd to have a "[name-N]"
        # We only send the N to the device in order to identify the targeted chip
        print("DEVICE: " + crnt_cmd.device_name)
        abbr_dev_cmd_str += " {}".format(devices[crnt_cmd.device_name]["chip_id"])

    # add args
    if crnt_cmd.arg1 is not None:
        abbr_dev_cmd_str += " {}".format(crnt_cmd.arg1)
    if crnt_cmd.arg2 is not None:
        abbr_dev_cmd_str += " {}".format(crnt_cmd.arg2)
    abbr_dev_cmd_str += "\n"
    dbg_pr_AgentMsgToDevice(verbose, abbr_dev_cmd_str)

    # If blockwrite cmd, need to veer off into a mini state machine of chunking.
    # crnt_cmd.arg2 will be a string of at least 8 long up to 1600
    # Incoming BW str will contain long data list Eg:
    # "BW c10 E1000000E1000000E1000001E1000001E1000001"
    if abbr_action_str == "BW":
        try:
            send_bw_data_to_mcu(crnt_cmd, ser_ch, ch_num, state, verbose)
        except blockwrite_chunk_excpn as bwe:
            print(bwe)
            raise
    else:
        # Write abbreviated cmd str to serial channel
        ser_ch.write_channel(ch_num, abbr_dev_cmd_str)

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
    global num_chips, devices

    # store info for ourselves
    num_chips = mcu_detect_reply_s.count(":") + 1
    mcu_reply_str_parts = mcu_detect_reply_s.split(':')
    reply_str = 'Detect '
    for i in range(num_chips):
        (name_n, bus_name, bus_addr, drvr_ctrl) = tuple(mcu_reply_str_parts[i].split(','))
        # Check name_n is of the form <name>-N as agent to mcu protocol uses N to identify the chip
        if '-' not in name_n and len(name_n.split('-')) != 2:
            raise bridge_excpn("name value in DETECT reply from device incorrect format: {}".format(name_n))
        devices[name_n] = dict()
        devices[name_n]["name"] = name_n
        devices[name_n]["chip_id"] = name_n.split('-')[1]
        devices[name_n]["name_n"] = name_n
        devices[name_n]["bus"] = bus_name
        devices[name_n]["bus_addr"] = bus_addr
        devices[name_n]["drvr_ctrl"] = drvr_ctrl

        reply_str += '{},{},{},DriverControl:{}="{}" '.format(name_n, devices[name_n]["bus"], devices[name_n]["bus_addr"], devices[name_n]["drvr_ctrl"], devices[name_n]["name"])

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
        recvd_data = sock.recv(SOCK_RX_BYTES)
        if not recvd_data:
            raise bridge_sock_excpn("No command received. Remote end may have terminated connection")
        return recvd_data
    except OSError as excp:
        print("Socket related error: {}".format(excp))
        raise excp

def wait_for_serial_data(ser_ch, ch_num):
    # smcio read is non-blocking so loop read until we read a full string
    data_str = ""
    time.sleep(1)
    while '\n' not in data_str:
        data_tmp = ''
        while data_tmp is '':
            data_tmp = ser_ch.read_channel(ch_num)
        data_str = data_str + data_tmp
    return data_str


def inner_loop(sock, ser_ch, ch_num, state, crnt_cmd, verbose):
    global wisce_device_id
    while True:
        try:
            dbg_pr_general(verbose, "Loop state: {}".format(state))
            if state == BRIDGE_STATE_HANDSHAKE_READ_REG0:
                # Initiate a Current Device "CD" command
                ser_ch.write_channel(ch_num, "CD\n")
                state = BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_READ_REG0
            elif state == BRIDGE_STATE_HANDSHAKE_WAIT_MCU_REPLY_READ_REG0:
                dbg_pr_general(verbose, "Initial read reg 0: Waiting for device reply")
                reply = wait_for_serial_data(ser_ch, ch_num)
                dbg_pr_DeviceMsgToAgent(verbose, reply)
                wisce_device_id = reply[:-1]
                print("wisce_device_id is {}".format(wisce_device_id))
                state = BRIDGE_STATE_HANDSHAKE_INFO
            elif state == BRIDGE_STATE_HANDSHAKE_INFO:
                # Create abbr Info cmd
                # Send MCU INFO abbr cmd
                ser_ch.write_channel(ch_num, "IN\n")
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
                crnt_cmd.new_cmd(cli_cmd_b)
                dbg_pr_ClientMsg(verbose, crnt_cmd.get_all_str())
                client_cmd_handler(crnt_cmd, ser_ch, ch_num, state, verbose)
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
        except KeyboardInterrupt as kbe:  # ToDo: Ctrl-C not working possibly due to socket
            sys.exit(1)
        except bridge_sock_excpn as bse:
            print("{}\n".format(bse))
            raise OSError
        ## TODO: send_bw_data_to_mcu() can raise exception!
        ## TODO: Should catch general Exception here, send ER msg to client & continue loop

def outer_loop(ser_ch, ch_num, verbose):
    while True:
        try:
            # Create socket & wait for connection to bridge client
            (bridgecli_sockcon, _) = init_bridge_socket()  # Blocks on socket conn
            state = BRIDGE_STATE_HANDSHAKE_READ_REG0
            current_cmd = current_command()

            with bridgecli_sockcon:
                inner_loop(bridgecli_sockcon, ser_ch, ch_num, state, current_cmd, verbose)

        except (TypeError, UnicodeError) as err:
            # Sometimes a client will send rubbish data down the socket
            # when a connection is closed, causing us to process garbage string.
            # Could be serious enough, start again with new connection
            print(type(err), err)
            print("Re-starting. Please attempt new WISCE connection")
        except KeyboardInterrupt as kbe:  # ToDo: Ctrl-C not working possibly due to socket
            sys.exit(1)
        except OSError as e:
            print("Socket related error: {}\nAny current operation will be aborted".format(e))


