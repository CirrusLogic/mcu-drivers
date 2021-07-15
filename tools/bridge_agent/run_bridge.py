#==========================================================================
# (c) 2021 Cirrus Logic, Inc.
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
sys.path.insert(1, '../smcio')
import smcio
import signal
import time
import bridge_agent
import traceback

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
        self.ser = serial.Serial(port, 115200, parity=serial.PARITY_NONE, rtscts=0, timeout=timeout)

    def write(self, byte_list):
        self.ser.write(byte_list)

    def read(self, length):
        byte_list = self.ser.read(length)
        if (len(byte_list) == 0):
            return None
        else:
            return byte_list

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument(dest='stdout_filename', type=str, help='The filename for stdout channel.')
    parser.add_argument(dest='bridge_filename', type=str, help='The filename for bridge channel.')
    parser.add_argument('-c', '--com-port', dest='comport', type=str, default=None, help='COM Port to use')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int, default='1', help='COM port timeout in seconds.')
    parser.add_argument('-p', '--packet-view', dest='packet_view', default=False, action='store_true')
    parser.add_argument('-v', '--verbose', dest='verbose', default=False, action='store_true')

    return parser.parse_args(args[1:])

def validate_args(args):
    return True

    # Check that stdout_filname path exists
    if ((args.stdout_filename != 'stdout') and (os.path.exists(args.stdout_filename))):
        print("Invalid stdout_filename path: " + args.stdout_filename)
        return False

    # Check that bridge_filename path exists
    if ((args.bridge_filename != 'stdout') and (os.path.exists(args.bridge_filename))):
        print("Invalid bridge_filename path: " + args.bridge_filename)
        return False

    return True

def print_start():
    print("")
    print("run_bridge")
    print("")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))

def print_args(args):
    print("")
    print("stdout_filename: " + args.stdout_filename)
    print("bridge_filename: " + args.bridge_filename)
    print("Timeout (s): " + str(args.timeout))
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
    else:
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
    if (args.stdout_filename != 'stdout'):
        f = open(args.stdout_filename, 'w')
        f.close()
    if (args.bridge_filename != 'stdout'):
        f = open(args.bridge_filename, 'w')
        f.close()

    # Create SMCIO processor and add channels for stdin/stdout, test, and coverage
    p = smcio.processor(com_port(args.comport, 115200, 0.001), args.packet_view)
    p.add_channel('0', channel_callback, (args.stdout_filename, '0'))
    p.add_channel('3', channel_callback, (args.bridge_filename, '3'))

    # Start SMCIO processor
    p.start()

    ''' Do Wisce Agent stuff using new module bridge_agent.py '''
    devices = dict()  # No device details discovered yet
    try:
        bridge_agent.outer_loop(p, '3', args.verbose)
    except IOError as e:
        print("\nIOError: {}. Exiting\n".format(e))
        raise
    except KeyboardInterrupt as e:
        print("\nHalted by User")
        sys.exit(1)
    except Exception as e:
        print("Uncaught exception: {}. Exception type: {}\n".format(e, type(e).__name__))
        traceback.print_exc()
    finally:
        # Stop SMCIO processor
        p.stop()
        print("Exit.")

if __name__ == "__main__":
    main(sys.argv)
