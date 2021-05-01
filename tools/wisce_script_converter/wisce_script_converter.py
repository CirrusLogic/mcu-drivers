# ==========================================================================
# (c) 2020 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Convert from WISCE Script File to Alt-OS Syscfg Reg output
# File    : wisce_to_syscfg_reg_converter.py
# --------------------------------------------------------------------------
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
# --------------------------------------------------------------------------
#
# Environment Requirements: None
#
# ==========================================================================

# ==========================================================================
# IMPORTS
# ==========================================================================
import os
import sys
import argparse
from wisce_script_importer import wisce_script_importer
from wisce_script_exporter_factory import wisce_script_exporter_factory, exporter_types

# ==========================================================================
# VERSION
# ==========================================================================
VERSION_STRING = "1.3.0"

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
supported_commands = exporter_types

# ==========================================================================
# CLASSES
# ==========================================================================

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-c', '--command', dest='command', type=str, choices=supported_commands, required=True, default=exporter_types[0],
                        help='The command you wish to execute.')
    parser.add_argument('-p', '--part', dest='part', type=str, required=True, help='The part number text for output.')
    parser.add_argument('-i', '--input', dest='input', type=str, required=True,
                        help='The filename of the WISCE script to be parsed.')
    parser.add_argument('-o', '--output', dest='output', type=str, default='.', help='The output filename.')
    parser.add_argument('-s', '--suffix', dest='suffix', type=str, default=None, help='The suffix to insert into output filename.')
    parser.add_argument('--include-comments', dest='include_comments', action="store_true",
                        help='Include comments from the WISCE script.')

    return parser.parse_args(args[1:])

def validate_args(args):
    # Check that input WISCE script exists
    if (not os.path.exists(args.input)):
        print("Invalid WISCE script path: " + args.input)
        return False

    return True


def print_start():
    print("")
    print("wisce_to_syscfg_reg_converter")
    print("Convert from WISCE Script Text file to Alt-OS Syscfg Reg")
    print("Version " + VERSION_STRING)

    return


def print_args(args):
    print("")
    print("Command: " + args.command)
    print("WISCE script path: " + args.input)
    if (args.output is not None):
        print("Output path: " + args.output)

    return


def print_results(results_string):
    print(results_string)

    return


def print_end():
    print("Exit.")

    return


def error_exit(error_message):
    print('ERROR: ' + error_message)
    exit(1)


# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
def main(argv):
    print_start()
    args = get_args(argv)
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    # Import WISCE script
    wsi = wisce_script_importer(args.input)

    # Create WISCE script exporter factory
    attributes = dict()
    attributes['part_number_str'] = args.part
    attributes['include_comments'] = args.include_comments
    attributes['output_path'] = args.output
    attributes['suffix'] = args.suffix
    wse = wisce_script_exporter_factory(attributes)

    # Based on command, add exporters
    if (args.command == 'c_array'):
        wse.add_exporter('c_array')

    # Export transaction list to exporter
    for t in wsi.get_transaction_list():
        wse.add_transaction(t)

    # Add metadata text
    metadata_text_lines = []
    metadata_text_lines.append('wisce_to_syscfg_reg_converter.py version: ' + VERSION_STRING)
    temp_line = ''
    for arg in argv:
        temp_line = temp_line + ' ' + arg
    metadata_text_lines.append('Command: ' + temp_line)

    for line in metadata_text_lines:
        wse.add_metadata_text_line(line)

    print_results(wse.to_file())
    print_end()

    return


if __name__ == "__main__":
    main(sys.argv)
