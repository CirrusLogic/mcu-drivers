#==========================================================================
# (c) 2022-2024, 2026 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Convert from HWT Files to C Header/Source
# File    : hwt_to_waveform_converter.py
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
import json
from hwt_to_waveform_templates import pwle_section, pwle_section_list, pwle_name_list, pcm_header, pcm_data

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
supported_part_numbers = ['cs40l26',
                          'cs40l26m',
                          'cs40l50',
                          'cs40l5x',]

host_init_haptics_msft_id_map = {
    'Hover'   : '0x1008',
    'Collide' : '0x1012',
    'Align'   : '0x1013',
    'Step'    : '0x1014',
    'Grow'    : '0x1015',
    'Press'   : '0x1006',
    'Release' : '0x1007',
    'Success' : '0x1009',
    'Error'   : '0x100A'
}

#==========================================================================
# CLASSES
#==========================================================================

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def get_args(args):
    optional_args_list = ['--wmdr', '--binary-input', '-s', '--suffix', '-i', '--i2c-address', '-b', '--block-size-limit',
                          '--sym-input', '--sym-output', '--binary', '--binary-output',
                          '--wmdr-only', '--exclude-wmfw', '--generic-sym', '--fw-img-version',
                          '--revision-check', '--sym-partition', '--no-sym-table',
                          '--exclude-dummy', '--skip-command-print', '--output-directory', '--host-init-haptics']

    rom_allowed_options = ['--wmdr', '--wmdr-only', '--exclude-wmfw', '--binary', '--binary-output']

    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument(dest='part_number', type=str, choices=supported_part_numbers,
                        help='The part number that the HWT is targeted at.')
    parser.add_argument(dest='hwt_filename', type=str,help='The HWT file to be parsed.')
    parser.add_argument('--output-directory', dest='output_directory', default='.', help="Output directory of files. By default uses current work dir")
    parser.add_argument('--host-init-haptics', dest='host_init_haptics', action='store_true', default=False,
                        help='Include hardcoded Microsoft haptics IDs in waveform output')

    return parser.parse_args(args[1:])


def validate_args(args):
    # Check that WMFW path exists
    if (not os.path.exists(args.hwt_filename)):
        print("Invalid HWT path: " + args.hwt_filename)
        return False

    # Check that output directory exists
    if (not os.path.exists(args.output_directory)):
        print("Invalid output directory path: " + args.output_directory)
        return False

    return True

def print_start():
    print("")
    print("hwt_to_waveform_converter")
    print("Convert from HWT File to C Header/Source")
    print("SDK version " + print_sdk_version(repo_path + '/sdk_version.h'))

    return

def print_args(args):
    print("")
    print("Part Number: " + args.part_number)
    print("HWT Path: " + args.hwt_filename)
    if (args.output_directory):
        print("Output Directory: " + args.output_directory)

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

#==========================================================================
# MAIN PROGRAM
#==========================================================================
def main(argv):
    print_start()

    if (not (validate_environment())):
        error_exit("Invalid Environment")

    args = get_args(argv)

    # validate arguments
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    f = open(args.hwt_filename)
    fwave_c = open(args.output_directory + "/waveforms.c", "w")
    fwave_h = open(args.output_directory + "/waveforms.h", "w")
    data = json.load(f)
    file_output_str = '#include "rth_types.h"\n#include <stddef.h>\n#include <stdlib.h>\n'
    file_h_output_str = '#include "rth_types.h"\n#include <stddef.h>\n#include <stdlib.h>\n\n'
    pwle_num = 1
    pcm_num = 1
    for entry in data["Entries"]:
        if entry["Header"]["Type"] == 'Type12':
            sec_num = 1
            pwle_str = ''
            for i in entry["Body"]["Sections"]:
                psec = pwle_section(pwle_num, sec_num, i["Time"]["Value"], i["Level"]["Value"], i["Frequency"]["Value"], str(i["ChirpMode"]["Value"]), str(i["Time"]["HalfCycles"]))
                pwle_str = pwle_str + str(psec) + '\n'
                sec_num += 1
            if(entry["Body"]["Length"]["HasLengthBeenCalculated"]):
                if("BrakingTimeInSeconds" in entry["MetaData"] and entry["MetaData"]["BrakingTimeInSeconds"] is not None):
                    lenInSeconds = entry["Body"]["LengthInSeconds"] + entry["MetaData"]["BrakingTimeInSeconds"]
                elif("LengthInSeconds" in entry["Body"]):
                    lenInSeconds = entry["Body"]["LengthInSeconds"]
                else:
                    lenInSeconds = 0
            else:
                lenInSeconds = 0
            if(args.host_init_haptics):
                try:
                    plist = pwle_section_list(pwle_num, entry["Body"]["NumberOfSections"]["Value"], int(lenInSeconds*1e6), entry["Body"]["Name"], msft_id=host_init_haptics_msft_id_map[entry["Body"]["Name"]])
                except KeyError:
                    plist = pwle_section_list(pwle_num, entry["Body"]["NumberOfSections"]["Value"], int(lenInSeconds*1e6), entry["Body"]["Name"], msft_id=0x0)
            else:
                plist = pwle_section_list(pwle_num, entry["Body"]["NumberOfSections"]["Value"], int(lenInSeconds*1e6), entry["Body"]["Name"])
            pwle_str = pwle_str + str(plist)
            file_output_str += pwle_str
            file_h_output_str += 'extern const rth_pwle_section_t *pwle' + str(pwle_num) + '[];\n'
            file_h_output_str += 'extern const uint32_t pwle_' + str(pwle_num) + '_size;\n\n'
            pwle_num += 1
        else:
            pcm_str = ''
            pcm_str = pcm_str + str(pcm_header(entry["Body"]["F0"]["Value"], entry["Body"]["ScaledReDc"]["Value"], pcm_num, args.part_number))
            pcm_str = pcm_str + str(pcm_data(entry["Body"]["Samples"], pcm_num)) + '\n'
            file_output_str += pcm_str
            file_h_output_str += 'extern uint16_t pcm_' + str(pcm_num) + '_f0;\n'
            file_h_output_str += 'extern uint16_t pcm_' + str(pcm_num) + '_redc;\n'
            file_h_output_str += 'extern uint8_t pcm_' + str(pcm_num)+ '_data[];\n'
            file_h_output_str += 'extern const uint32_t pcm_' + str(pcm_num) + '_data_size;\n\n'
            pcm_num += 1

    if(pwle_num != 1):
        file_h_output_str += 'extern const uint32_t pwleCount;\n'
        file_h_output_str += 'extern rth_pwle_t *pwleList[];\n'
        file_output_str += f"uint32_t const pwleCount = {pwle_num-1};"
        file_output_str += f'\nconst rth_pwle_t *pwleList[{pwle_num-1}] =\n{{'
        for i in range(1, pwle_num):
            file_output_str += f"\n\t&pwle_{i},"
        file_output_str += "\n};\n"

    fwave_c.write(file_output_str)
    fwave_h.write(file_h_output_str)

    results_str = ''

    print_results(results_str)

    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
