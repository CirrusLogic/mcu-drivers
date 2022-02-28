#==========================================================================
# (c) 2021-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project :
# File    : create_ide_projects.py
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
import shutil
from pathlib import Path

#==========================================================================
# VERSION
#==========================================================================

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
eclipse_files_to_copy = {'.cproject': ['', 0],
                         '.project': ['', 0],
                         'live_oak_jlink.launch': ['', 0],
                         'live_oak_stlink.launch': ['', 0],
                         'holdout_jlink.launch': ['', 0],
                         'holdout_stlink.launch': ['', 0],
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
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-e', '--eclipse-only', dest='eclipse_only', action="store_true", help='Only copy Eclipse project files')

    return parser.parse_args(args[1:])

def validate_args(args):
    return True

def print_start():
    print("")
    print("create_ide_projects")
    print("")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))

    return

def print_args(args):
    print("")
    print("eclipse_only: " + str(args.eclipse_only))
    print("")

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
    results_str = ''

    print_start()

    if (not (validate_environment())):
        error_exit("Invalid Environment")

    args = get_args(argv)

    # validate arguments
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    # Get set of files to copy - for now only Eclipse projects supported
    all_project_files = eclipse_files_to_copy
    input_dir = os.path.dirname(os.path.realpath(__file__))
    output_dir = Path(input_dir)
    output_dir = output_dir.parent.absolute()
    output_dir = str(output_dir.parent.absolute())
    repo_folder_name = os.path.basename(output_dir)
    for key in all_project_files:
        input_file = input_dir + os.path.sep + key
        output_file = output_dir + all_project_files[key][0] + os.path.sep + key
        shutil.copyfile(input_file, output_file)
        f = open(output_file, 'r')
        lines = f.readlines()
        f.close()
        for i in range(0, len(lines)):
            if '{repo_folder_name}' in lines[i]:
                lines[i] = lines[i].replace('{repo_folder_name}', repo_folder_name)
                all_project_files[key][1] += 1

        f = open(output_file, 'w')
        f.writelines(lines)
        f.close()

    results_str += "Copied files/Updated lines:\n"
    for key in all_project_files:
        results_str += key + '/' + str(all_project_files[key][1]) + '\n'

    print_results(results_str)
    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
