#==========================================================================
# (c) 2019 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Convert from WMFW/WMDR ("BIN") Files to C Header/Source
# File    : firmware_converter.py
#--------------------------------------------------------------------------
# Redistribution and use of this file in source and binary forms, with
# or without modification, are permitted.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
import argparse
from wmfw_parser import wmfw_parser, get_memory_region_from_type
from wmdr_parser import wmdr_parser
from c_h_file_templates import header_file, source_file

#==========================================================================
# VERSION
#==========================================================================
VERSION_STRING = "1.1.0"

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
supported_part_numbers = ['cs35l41', 'cs40l25']
supported_commands = ['print', 'export']

supported_mem = {
    'cs35l41': {
        'xm': {
            'u24': 0x2800000,
            'p32': 0x2000000,
            'u32': 0x2400000,
        },
        'ym': {
            'u24': 0x3400000,
            'p32': 0x2C00000,
            'u32': 0x3000000,
        },
        'pm': {
            'p32': 0x3800000,
        }
    },
    'cs40l25': {
        'xm': {
            'u24': 0x2800000,
            'p32': 0x2000000,
            'u32': 0x2400000,
        },
        'ym': {
            'u24': 0x3400000,
            'p32': 0x2C00000,
            'u32': 0x3000000,
        },
        'pm': {
            'p32': 0x3800000,
        }
    }    
}

block_size_limit = 4140

#==========================================================================
# CLASSES
#==========================================================================
class address_resolver:
    def __init__(self, part_number):
        self.mem_map = supported_mem[part_number]

        return

    def resolve(self, mem_region, mem_type, offset):
        address = None
        if ((mem_region in self.mem_map) and (mem_type in self.mem_map[mem_region])):
            if (mem_type == 'u24'):
                addresses_per_word = 4
            elif (mem_type == 'p32'):
                addresses_per_word = 3

            address = self.mem_map[mem_region][mem_type] + offset * addresses_per_word

            # All addresses must be on 4-byte boundaries
            address = address & ~0x3

        return address

class block_list:
    def __init__(self, size_limit, address_resolver):
        self.size_limit = size_limit
        self.ar = address_resolver
        self.blocks = []

        return

    def rehash_blocks(self):
        new_blocks = []
        for block in self.blocks:
            temp_len = len(block[1])
            if (temp_len < self.size_limit):
                new_blocks.append((block[0], block[1]))
            else:
                temp_block = []
                temp_start_offset = block[0]
                for data_byte in block[1]:
                    temp_block.append(data_byte)
                    if (len(temp_block) >= self.size_limit):
                        new_blocks.append((temp_start_offset, temp_block))
                        temp_start_offset = temp_start_offset + len(temp_block)
                        temp_block = []
                if (len(temp_block) > 0):
                    new_blocks.append((temp_start_offset, temp_block))

        self.blocks = new_blocks

        return

class fw_block_list(block_list):
    def __init__(self, data_blocks, size_limit, address_resolver):
        block_list.__init__(self, size_limit, address_resolver)

        for block in data_blocks:
            temp_mem_region = get_memory_region_from_type(block.fields['type'])
            temp_offset = block.fields['start_offset']
            new_address = self.ar.resolve(temp_mem_region, block.memory_type, temp_offset)
            self.blocks.append((new_address, block.data))
        return

class coeff_block_list(block_list):
    def __init__(self, data_blocks, size_limit, address_resolver, fw_id_block):
        block_list.__init__(self, size_limit, address_resolver)

        for block in data_blocks:
            temp_mem_region = get_memory_region_from_type(block.fields['type'])
            # Coefficient value data blocks in WMDR files offsets are in terms of External Port address rather than
            # in terms of algorithm memory region fields (i.e. XM/YM words), so calculation is different
            temp_offset = fw_id_block.get_adjusted_offset(block.fields['algorithm_identification'],
                                                          temp_mem_region,
                                                          0)
            new_address = self.ar.resolve(temp_mem_region, block.memory_type, temp_offset)
            new_address = new_address + block.fields['start_offset']
            self.blocks.append((new_address, block.data))
        return


#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def get_args():
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-s', '--suffix', type=str, default='',
                        dest='suffix', help='Add a suffix to filenames, variables and defines.')
    parser.add_argument('command', type=str, choices=supported_commands,
                        help='The command you wish to execute.')
    parser.add_argument('part_number', type=str, choices=supported_part_numbers,
                        help='The part number that the wmfw is targeted at.')
    parser.add_argument('wmfw', type=str, help='The wmfw (or \'firmware\') file to be parsed.')
    parser.add_argument('wmdrs', type=str, nargs='*', help='The wmdr (or \'bin\') file(s) to be '\
                        'parsed.')

    args = parser.parse_args()
    return args

def validate_args(args):
    # Check that WMFW path exists
    if (not os.path.exists(args.wmfw)):
        print("Invalid wmfw path: " + args.wmfw)
        return False
    if (len(args.wmdrs) == 0):
        return True
    # Check that WMDR path(s) exists
    for wmdr in args.wmdrs:
        if (not os.path.exists(wmdr)):
            print("Invalid wmdr path: " + wmdr)
            return False

    return True

def print_start():
    print("")
    print("firmware_converter")
    print("Convert from WMFW/WMDR (\"BIN\") Files to C Header/Source")
    print("Version " + VERSION_STRING)

    return

def print_args(args):
    print("")
    print("Command: " + args.command)
    print("Part Number: " + args.part_number)
    print("WMFW Path: " + args.wmfw)
    if (len(args.wmdrs) > 0):
        for wmdr in args.wmdrs:
            print("WMDR Path: " + wmdr)

    if (args.suffix):
        print("Suffix: " + args.suffix)
    else:
        print("No suffix")

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

    args = get_args()

    # validate arguments
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    if (len(args.wmdrs) > 0):
        process_wmdr = True
    else:
        process_wmdr = False

    # Parse WMFW and WMDR files
    wmfw = wmfw_parser(args.wmfw)
    wmfw.parse()

    if (process_wmdr):
        wmdrs = []
        for wmdr_filename in args.wmdrs:
            wmdr = wmdr_parser(wmdr_filename)
            wmdr.parse()
            wmdrs.append(wmdr)

    suffix = ""
    if (args.suffix):
        suffix = "_" + args.suffix

    # Create address resolver
    res = address_resolver(args.part_number)

    # Create firmware data blocks - size according to 'block_size_limit'
    fw_data_block_list = fw_block_list(wmfw.get_data_blocks(), block_size_limit, res)
    fw_data_block_list.rehash_blocks()

    # Create coeff data blocks - size according to 'block_size_limit'
    coeff_data_block_lists = []
    if (process_wmdr):
        for wmdr in wmdrs:
            coeff_data_block_list = coeff_block_list(wmdr.coefficient_value_data_blocks,
                                                     block_size_limit,
                                                     res,
                                                     wmfw.fw_id_block)
            coeff_data_block_list.rehash_blocks()
            coeff_data_block_lists.append(coeff_data_block_list)


    # Create C Header
    hf = header_file(args.part_number + suffix)
    if (not process_wmdr):
        hf.update_block_info(len(fw_data_block_list.blocks), None)
    else:
        coeff_data_block_list_lengths = []
        for coeff_data_block_list in coeff_data_block_lists:
            coeff_data_block_list_lengths.append(len(coeff_data_block_list.blocks))
        hf.update_block_info(len(fw_data_block_list.blocks), coeff_data_block_list_lengths)
    # Add controls
    # For each algorithm information data block
    for alg_block in wmfw.get_algorithm_information_data_blocks():
        # For each 'coefficient_descriptor', create control name and resolve address
        for coeff_desc in alg_block.fields['coefficient_descriptors']:
            temp_mem_region = get_memory_region_from_type(coeff_desc.fields['type'])
            temp_coeff_offset = wmfw.fw_id_block.get_adjusted_offset(alg_block.fields['algorithm_id'],
                                                                     temp_mem_region,
                                                                     coeff_desc.fields['start_offset'])
            temp_coeff_address = res.resolve(temp_mem_region, 'u24', temp_coeff_offset)

            # Re-name if algorithm is the overall system control algorithms
            if (alg_block.fields['algorithm_id'] == wmfw.fw_id_block.fields['firmware_id']):
                algorithm_name = "GENERAL"
            else:
                algorithm_name = alg_block.fields['algorithm_name']
            # Add control
            hf.add_control(algorithm_name, coeff_desc.fields['coefficient_name'], temp_coeff_address)

    # Create C Source
    cf = source_file(args.part_number + suffix)
    # Add FW Blocks
    for block in fw_data_block_list.blocks:
        block_bytes = []
        for byte_str in block[1]:
            block_bytes.append(int.from_bytes(byte_str, 'little', signed=False))
        cf.add_fw_block(block[0], block_bytes)
    # Add Coeff Blocks
    if (process_wmdr):
        coeff_block_list_count = 0
        for coeff_data_block_list in coeff_data_block_lists:
            for block in coeff_data_block_list.blocks:
                # Convert from list of bytestring to list of int
                block_bytes = []
                for byte_str in block[1]:
                    block_bytes.append(int.from_bytes(byte_str, 'little', signed=False))

                cf.add_coeff_block(coeff_block_list_count, block[0], block_bytes)
            coeff_block_list_count = coeff_block_list_count + 1

    results_str = ''
    if (args.command == 'print'):
        results_str = '\n'
        results_str = results_str + 'WMFW File: ' + args.wmfw + '\n'
        results_str = results_str + str(wmfw) + "\n"
        if (process_wmdr):
            wmdr_count = 0
            for wmdr in wmdrs:
                results_str = results_str + 'WMDR File: ' + args.wmdrs[wmdr_count] + '\n'
                results_str = results_str + str(wmdr)
                wmdr_count = wmdr_count + 1
    elif (args.command == 'export'):
        results_str = 'Exported to files:\n'

        # Write output to filesystem
        temp_filename = args.part_number + suffix + "_firmware.h"
        f = open(temp_filename, 'w')
        f.write(str(hf))
        f.close()
        results_str = results_str + temp_filename + '\n'

        temp_filename = args.part_number + suffix + "_firmware.c"
        f = open(temp_filename, 'w')
        f.write(str(cf))
        f.close()
        results_str = results_str + temp_filename + '\n'

    print_results(results_str)

    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
