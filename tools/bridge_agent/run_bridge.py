#==========================================================================
# (c) 2021-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project :
# File    : run_bridge.py
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

#==========================================================================
# IMPORTS
#==========================================================================
import os
import sys
repo_path = os.path.dirname(os.path.abspath(__file__)) + '/../..'
sys.path.insert(1, (repo_path + '/tools/sdk_version'))
from sdk_version import print_sdk_version
import argparse
import serial
import serial.tools.list_ports
sys.path.insert(1, '../smcio')
import smcio
import signal
import time
import bridge_agent
import traceback
import re


#==========================================================================
# VERSION
#==========================================================================

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
timeout_counter = 0

#==========================================================================
# CLASSES
#==========================================================================
class com_port(smcio.serial_io_interface):

    def __init__(self, port, speed, timeout):
        self.ser = serial.Serial(port, speed, parity=serial.PARITY_NONE, rtscts=0, timeout=timeout)

    def write(self, byte_list):
        self.ser.write(byte_list)

    def read(self, length):
        byte_list = self.ser.read(length)
        if (len(byte_list) == 0):
            return None
        else:
            return byte_list


class host_platform:
    win_format_regex = "(COM[0-9]+)"
    wsl_format_regex = "(/dev/ttyS[0-9]+)"
    win_autdetect_regex = "STLink.+(COM[0-9]+)"
    win_str = 'win32'
    wsl_str = 'linux'
    platform = sys.platform
    autodetect_port = None
    can_autodetect = False

    @classmethod
    def check_platform_support(cls):
        if re.search(cls.platform, cls.win_str, re.IGNORECASE):
            cls.comport_regex_cp = re.compile(cls.win_format_regex)
            cls.can_autodetect = True
        elif re.search(cls.platform, cls.wsl_str, re.IGNORECASE):
            cls.comport_regex_cp = re.compile(cls.wsl_format_regex)
            cls.can_autodetect = False
        else:
            print("Unsupported platform")
            return False
        return True

    @classmethod
    def check_user_com_port_format(cls, user_com_port_str):
        if not cls.check_platform_support():
            return False
        if cls.comport_regex_cp.search(user_com_port_str) is not None:
            cls.checked_user_com_port_str = user_com_port_str
            return True
        else:
            return False

    @classmethod
    def autodetect_stlink_com_port(cls):
        if not cls.check_platform_support():
            return False
        # Currently can only auto-detect on Windows
        # ToDo: add support for Mac OS and Linux
        if not cls.can_autodetect:
            print("Can not autodetect COM port on {}.\nPlease supply valid com port".format(cls.platform))
        else:
            comstring_cp = re.compile(cls.win_autdetect_regex)
            for s in serial.tools.list_ports.comports():
                results = comstring_cp.search(s.description)
                if results != None:
                    cls.autodetect_port = results.group(1)
                    print("STLink auto-detected: {}".format(cls.autodetect_port))
                    break
        return cls.autodetect_port

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-c', '--com-port', dest='comport', type=str, default=None,
                        help='COM Port to use Eg COM8 (for Windows) or /dev/ttyS8 for WSL. '
                             'If omitted autodetection will be attempted for Windows hosts only')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int, default='1', help='COM port timeout in seconds.')
    parser.add_argument('-p', '--packet-view', dest='packet_view', default=False, action='store_true')
    parser.add_argument('-v', '--verbose', dest='verbose', default=False, action='store_true')
    parser.add_argument('-s', '--stdout_filename', dest="stdout_filename", type=str, help='The filename for stdout channel.')
    parser.add_argument('-b', '--bridge_filename', dest='bridge_filename', type=str, help='The filename for bridge channel.')
    parser.add_argument('-r', '--user_num_reg_in_chunk', dest='user_num_reg_in_chunk', default=100, type=int,
                        help='The number of registers to chunk in a block-write operation. '
                        'Must be between 1 and 200. Omitting this option defaults to 100')

    return parser.parse_args(args[1:])

def validate_args(args):
    # Check that stdout_filname path exists
    if args.stdout_filename != 'stdout' and  args.stdout_filename != None and \
        not os.path.exists(os.path.split(os.path.realpath(args.stdout_filename))[0]):
        print("Invalid stdout_filename path: " + args.stdout_filename)
        return False

    # Check that bridge_filename path exists
    if args.bridge_filename != 'stdout' and args.bridge_filename != None and \
        not os.path.exists(os.path.split(os.path.realpath(args.bridge_filename))[0]):
        print("Invalid bridge_filename path: " + args.bridge_filename)
        return False

    if args.comport is not None:
        if not host_platform().check_user_com_port_format(args.comport):
            print("Invalid comport: {}".format(args.comport))
            return False
    else:
        print ("comport not supplied, attempting to autodetect ...")
        autodetect_com_port = host_platform().autodetect_stlink_com_port()
        if autodetect_com_port is not None:
            args.comport = autodetect_com_port
        else:
            print("Could not autodetect com port")
            return False

    # Check register chunk is legal
    # NB: 200 is dictated by the current buffer size at the MCU for receiving register write values
    if not 0 < args.user_num_reg_in_chunk <= 200:
        print("Invalid chunk-size specified ({})".format(args.user_num_reg_in_chunk))
        return False

    return True

def print_start():
    print("")
    print("run_bridge")
    print("")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))
    print("Process Id: {}".format(os.getpid()))

def print_args(args):
    print("")
    print("stdout_filename: {}".format(args.stdout_filename if args.stdout_filename is not None else "None"))
    print("bridge_filename: {}".format(args.bridge_filename if args.bridge_filename is not None else "None"))
    print("Timeout (s): " + str(args.timeout))
    if args.verbose:
        print("Register chunk size for block-writes: {}".format(args.user_num_reg_in_chunk))
    print("")

def print_results(results_string):
    print(results_string)



def error_exit(error_message):
    print('ERROR: ' + error_message)
    exit(1)

def channel_callback(callback_arg, packet_string):
    global timeout_counter

    filename = callback_arg[0]
    id = callback_arg[1]

    timeout_counter = 0

    if (filename == 'stdout'):
        packet_string = packet_string.replace('\n', ('\n' + str(id) + '>'))
        print(str(id) + '>', end='')
        print(packet_string, end='')
        print('\n')
    elif (filename != None):
        f = open(filename, 'a')
        f.write(packet_string)
        f.close()


#==========================================================================
# MAIN PROGRAM
#==========================================================================
def main(argv):
    global timeout_counter

    print_start()

    if (not (validate_environment())):
        error_exit("Invalid Environment")

    args = get_args(argv)

    # validate arguments
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    # Clear any files
    if (args.stdout_filename != 'stdout' and args.stdout_filename != None):
        f = open(args.stdout_filename, 'w')
        f.close()
    if (args.bridge_filename != 'stdout' and args.bridge_filename != None):
        f = open(args.bridge_filename, 'w')
        f.close()

    # Create SMCIO processor and add channels for stdin/stdout, test, and coverage
    p = smcio.processor(com_port(args.comport, 115200, args.timeout),
                        bridge_agent.PAYLOAD_UNPACK_SHORT,
                        bridge_agent.PAYLOAD_UNPACK_INT,
                        args.packet_view)
    p.add_channel('0', channel_callback, (args.stdout_filename, '0'))
    p.add_channel('3', channel_callback, (args.bridge_filename, '3'))

    # Start SMCIO processor
    p.start()

    ''' Do Wisce Agent stuff using new module bridge_agent.py '''
    devices = dict()  # No device details discovered yet
    try:
        bridge_agent.outer_loop(p, '3', args.verbose, args.user_num_reg_in_chunk)
    except IOError as e:
        print("\nIOError: {}. Exiting\n".format(e))
        raise
    except KeyboardInterrupt as e:
        print("\nHalted by User")
        os.kill(os.getpid(), signal.SIGINT)
    except Exception as e:
        print("Uncaught exception: {}. Exception type: {}\n".format(e, type(e).__name__))
        traceback.print_exc()
    finally:
        # Stop SMCIO processor
        p.stop()
        print("Exit.")

if __name__ == "__main__":
    main(sys.argv)
