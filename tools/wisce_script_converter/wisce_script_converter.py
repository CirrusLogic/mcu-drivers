# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
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
repo_path = os.path.dirname(os.path.abspath(__file__)) + '/../..'
sys.path.insert(1, (repo_path + '/tools/sdk_version'))
from sdk_version import print_sdk_version
import argparse
import script_importer
from wisce_script_exporter_factory import wisce_script_exporter_factory, exporter_types

# ==========================================================================
# VERSION
# ==========================================================================

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
                        help='The filename of the WISCE script or CSV file to be parsed. CSV filenames must end with a .csv extension')
    parser.add_argument('-o', '--output', dest='output', type=str, default='.', help='The output filename.')
    parser.add_argument('-s', '--suffix', dest='suffix', type=str, default=None, help='The suffix to insert into output filename.')
    parser.add_argument('--include-comments', dest='include_comments', action="store_true",
                        help='Include comments from the WISCE script.')
    parser.add_argument('-sym', '--symbol_file', dest='symbol_file', type=str, default=None,
                        help='The filename containing firmware symbols, produced by firmware_converter. ' +
                             'Needed to differentiate between normal named registers and firmware registers.')

    return parser.parse_args(args[1:])

def validate_args(args):
    # Check that input WISCE script or CSV file exists
    if (not os.path.exists(args.input)):
        print("Invalid WISCE script or CSV file path: " + args.input)
        return False

    return True


def print_start():
    print("")
    print("wisce_to_syscfg_reg_converter")
    print("Convert from either WISCE Script Text file or SCS generated CSV file to Alt-OS Syscfg Reg")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))

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

    # Import WISCE script or CSV file
    if args.input.lower().endswith('.csv'):
        script_imp = script_importer.scs_csv_script_importer(args.input, args.command, args.symbol_file)
    else:
        script_imp = script_importer.wisce_script_importer(args.input, args.command, args.symbol_file)

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
    elif (args.command == 'c_functions'):
        wse.add_exporter('c_functions')

    # Export transaction list to exporter
    for t in script_imp.get_transaction_list():
        wse.add_transaction(t)

    # Add metadata text
    metadata_text_lines = []
    metadata_text_lines.append('wisce_to_syscfg_reg_converter.py SDK version: ' + print_sdk_version(repo_path + '/sdk_version.h'))
    temp_line = ''
    for arg in argv:
        temp_line = temp_line + ' ' + arg
    metadata_text_lines.append('Command: ' + temp_line)

    for line in metadata_text_lines:
        wse.add_metadata_text_line(line)

    print_results(wse.to_file())
    print_end()

    return 0


if __name__ == "__main__":
    main(sys.argv)
