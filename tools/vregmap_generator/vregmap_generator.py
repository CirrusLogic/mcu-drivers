# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Generate vregmap.h/vregmap.c from WISCE/Soundclear Studio device XML
# File    : vregmap_generator.py
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
from vregmap_wisce_xml_importer import vregmap_wisce_xml_importer
from vregmap_scs_xml_importer import vregmap_scs_xml_importer
import xml.etree.ElementTree as ET
from vregmap_exporter import vregmap_exporter

# ==========================================================================
# VERSION
# ==========================================================================

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================

# ==========================================================================
# CLASSES
# ==========================================================================

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-c', '--command', dest='command', type=str, choices=["print", "export"], required=True, default="print",
                        help='The command you wish to execute.')
    parser.add_argument('-i', '--input', dest='input', type=str, required=True,
                        help='The filename of the XML to be parsed.')
    parser.add_argument('-o', '--output_dir', dest='output_dir', type=str, default='.', help='The path to output directory.')

    return parser.parse_args(args[1:])

def validate_args(args):
    # Check that input XML exists
    if (not os.path.exists(args.input)):
        print("Invalid XML path: " + args.input)
        return False

    # Check that output directory exists:
    if (not os.path.exists(args.output_dir)):
        print("Invalid output directory: " + args.output_dir)
        return False

    return True


def print_start():
    print("")
    print("vregmap_generator")
    print("Generate vregmap.h/vregmap.c from WISCE/Soundclear Studio device XML")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))

    return


def print_args(args):
    print("")
    print("Command: " + args.command)
    print("Input XML path: " + args.input)
    print("Output directory: " + args.output_dir)

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

def get_xml_type(fn):
    type = "WISCE"

    tempXmlP = ET.XMLParser(encoding="utf-8")
    tempTree = ET.parse(fn, parser=tempXmlP)
    tempRoot = tempTree.getroot()

    if (tempRoot.tag == "SCSDevice"):
        type = "SCS"

    return type
# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
def main(argv):
    print_start()
    args = get_args(argv)
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    if (get_xml_type(args.input) == "WISCE"):
        i = vregmap_wisce_xml_importer(args.input)
    else:
        i = vregmap_scs_xml_importer(args.input)
    vdevice = i.get_device()

    if (args.command == "print"):
        print(vdevice)
    else:
        metadata_text_lines = []
        metadata_text_lines.append('vregmap_generator.py SDK version: ' + print_sdk_version(repo_path + '/sdk_version.h'))
        temp_line = ''
        for arg in argv:
            temp_line = temp_line + ' ' + arg
        metadata_text_lines.append('Command: ' + temp_line)
        e = vregmap_exporter(args.output_dir, vdevice, metadata_text_lines)
        e.export()

    print_results("")
    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
