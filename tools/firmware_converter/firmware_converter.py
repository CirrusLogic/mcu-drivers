#==========================================================================
# (c) 2019-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Convert from WMFW/WMDR ("BIN") Files to C Header/Source
# File    : firmware_converter.py
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
from wmfw_parser import wmfw_parser, get_memory_region_from_type
from wmdr_parser import wmdr_parser
from binary_parser import bin_parser
from firmware_exporter_factory import firmware_exporter_factory

#==========================================================================
# VERSION
#==========================================================================

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

supported_part_numbers = ['cs35l41', 'cs40l25', 'cs40l30', 'cs48l32', 'cs47l63', 'cs47l66', 'cs47l67', 'cs47l15', 'cs47l35_dsp1', 'cs47l35_dsp2', 'cs47l35_dsp3', 'cs40l26']

supported_commands = ['print', 'export', 'wisce', 'fw_img_v1', 'fw_img_v2', 'json']

supported_mem_maps = {
    'halo_type_0': {
        'parts': ['cs35l41', 'cs40l25', 'cs40l30', 'cs48l32', 'cs47l63', 'cs47l66', 'cs47l67', 'cs40l26'],
        'xm': {
            'u24': (0x2800000, 0x2bfffff),
            'p32': (0x2000000, 0x23fffff),
            'u32': (0x2400000, 0x27fffff),
        },
        'ym': {

            'u24': (0x3400000, 0x37fffff),
            'p32': (0x2c00000, 0x2ffffff),
            'u32': (0x3000000, 0x33fffff),
        },
        'pm': {
            'pm32': (0x3800000, 0xfffffff),
        }
    },

    'adsp_dsp_1': {
        'parts': ['cs47l15', 'cs47l35_dsp1'],
        'xm': {
            'u24': (0xa0000, 0xbffff),
        },
        'ym': {
            'u24': (0xc0000, 0xdffff),
        },
        'zm': {
            'u24': (0xe0000, 0xfffff),
        },
        'pm': {
            'pm32': (0x80000, 0x9ffff),
        }
    },
    'adsp_dsp_2': {
        'parts': ['cs47l35_dsp2'],
        'xm': {
            'u24': (0x120000, 0x13ffff),
        },
        'ym': {
            'u24': (0x140000, 0x15ffff),
        },
        'zm': {
            'u24': (0x160000, 0xffffff),
        },
        'pm': {
            'pm32': (0x100000, 0x11ffff),
        },
    },
    'adsp_dsp_3': {
        'parts': ['cs47l35_dsp3'],
        'xm': {
            'u24': (0x1a0000, 0x1bffff),
        },
        'ym': {
            'u24': (0x1c0000, 0x1dffff),
        },
        'zm': {
            'u24': (0x1e0000, 0xffffff),
        },
        'pm': {
            'pm32': (0x180000, 0x19ffff),
        }
    }
}

#==========================================================================
# CLASSES
#==========================================================================
class address_resolver:
    def __init__(self, part_number):
        for key in supported_mem_maps.keys():
            if (part_number in supported_mem_maps[key]['parts']):
                self.mem_map = supported_mem_maps[key]
                self.core = key

        return

    def resolve(self, mem_region, mem_type, offset):
        address = None
        if ((mem_region in self.mem_map) and (mem_type in self.mem_map[mem_region])):
            if (self.core == 'halo_type_0'):
                if (mem_type == 'u24'):
                    addresses_per_word = 4
                elif (mem_type == 'p32'):
                    addresses_per_word = 3
                elif (mem_type == 'pm32'):
                    addresses_per_word = 5
            else:
                if (mem_type == 'u24'):
                    addresses_per_word = 2
                elif (mem_type == 'pm32'):
                    addresses_per_word = 3

            address = self.mem_map[mem_region][mem_type][0] + offset * addresses_per_word

            # Packed addresses must be on 4-byte boundaries
            if (self.core == 'halo_type_0' and mem_type == 'p32'):
                address = address & ~0x3

        return address

    def unresolve(self, address):
        for mem_region in self.mem_map:
            if mem_region != 'parts':
                for mem_type in self.mem_map[mem_region]:
                    if address >= self.mem_map[mem_region][mem_type][0] and address < self.mem_map[mem_region][mem_type][1]:
                        return (mem_type, self.mem_map[mem_region][mem_type][0])
        error_exit("Target address outside of memory range")

    def bytes_per_addr(self):
        if (self.core == 'halo_type_0'):
            return 1
        else:
            return 2

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
            if (temp_len <= self.size_limit):
                new_blocks.append((block[0], block[1]))
            else:
                temp_block = []
                temp_start_offset = block[0]
                idx = 0
                while (idx < temp_len):
                    next_idx = min(self.size_limit, temp_len - idx) + idx
                    temp_block.extend(block[1][idx:next_idx])
                    idx = next_idx
                    new_blocks.append((temp_start_offset, temp_block))
                    temp_start_offset = temp_start_offset + (len(temp_block) // self.ar.bytes_per_addr())
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
            if (temp_mem_region != 'abs'):
                new_address = self.ar.resolve(temp_mem_region, block.memory_type, block.fields['start_offset'])
            else:
                new_address = block.fields['start_offset']
            self.blocks.append((new_address, block.data))
        return

class coeff_block_list(block_list):
    def __init__(self, data_blocks, size_limit, address_resolver, fw_id_block):
        block_list.__init__(self, size_limit, address_resolver)

        for block in data_blocks:
            temp_mem_region = get_memory_region_from_type(block.fields['type'])
            if (temp_mem_region != 'abs'):
                # Coefficient value data blocks in WMDR files offsets are in terms of External Port address rather than
                # in terms of algorithm memory region fields (i.e. XM/YM words), so calculation is different
                temp_offset = fw_id_block.get_adjusted_offset(block.fields['algorithm_identification'],
                                                              temp_mem_region,
                                                              0)
                new_address = self.ar.resolve(temp_mem_region, block.memory_type, temp_offset)
                new_address = new_address + block.fields['start_offset']
            else:
                new_address = block.fields['start_offset']
            self.blocks.append((new_address, block.data))

        return

class bin_block_list(block_list):
    def __init__(self, data_blocks, size_limit, address_resolver):
        block_list.__init__(self, size_limit, address_resolver)

        for block in data_blocks:
            new_address = block.fields['address']
            self.blocks.append((new_address, block.data))
        return


#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument(dest='command', type=str, choices=supported_commands, help='The command you wish to execute.')
    parser.add_argument(dest='part_number', type=str, choices=supported_part_numbers,
                        help='The part number that the wmfw is targeted at.')
    parser.add_argument(dest='wmfw', type=str,help='The wmfw (or \'firmware\') file to be parsed.')
    parser.add_argument('--wmdr', dest='wmdrs', type=str, nargs='*', help='The wmdr file(s) to be parsed.')
    parser.add_argument('--binary-input', dest='bins', type=str, nargs='*', help='The bin file(s) to be parsed and their addresses. Format: "<hex addr>,<bin filename>", individual files separated by whitespace')
    parser.add_argument('-s', '--suffix', type=str, default='',
                        dest='suffix', help='Add a suffix to filenames, variables and defines.')
    parser.add_argument('-i', '--i2c-address', type=str, default='0x80', dest='i2c_address', help='Specify I2C address for WISCE script output.')
    parser.add_argument('-b', '--block-size-limit', type=int, default='4140', dest='block_size_limit', help='Specify maximum byte size of block per control port transaction.  Can be no larger than 4140.')
    parser.add_argument('--sym-input', dest='symbol_id_input', type=str, default=None, help='The location of the symbol table C header(s).  If not specified, a header is generated with all controls.')
    parser.add_argument('--sym-output', dest='symbol_id_output', type=str, default=None, help='The location of the output symbol table C header.  Only used when no --sym-input is specified.')
    parser.add_argument('--binary', dest='binary_output', action="store_true", help='Request binary fw_img output format. WARNING: --binary is going to be depracated soon, please use --binary-output.')
    parser.add_argument('--binary-output', dest='binary_output', action="store_true", help='Request binary fw_img output format.')
    parser.add_argument('--wmdr-only', dest='exclude_wmfw', action="store_true", help='(To be depracated, please use --exclude-wmfw) Request to ONLY store WMDR files in fw_img.')
    parser.add_argument('--exclude-wmfw', dest='exclude_wmfw', action="store_true", help='Request to ONLY store WMDR/bin files in fw_img.')
    parser.add_argument('--generic-sym', dest='generic_sym', action="store_true", help='Use generic algorithm name for \'FIRMWARE_*\' algorithm controls')
    parser.add_argument('--fw-img-version', type=lambda x: int(x,0), default='0', dest='fw_img_version', help='Release version for the fw_img that ties together a WMFW fw revision with releases of BIN files. Accepts type int of any base.')
    parser.add_argument('--revision-check', dest='revision_check', action="store_true", help='Request to fail if WMDR FW revision does not match WMFW')
    parser.add_argument('--sym-partition', dest='sym_partition', action="store_true", help='Partition symbol IDs by algorithm so new symbols added to one algorithm don\'t cause subsequent IDs to be shifted')
    parser.add_argument('--no-sym-table', dest='no_sym_table', action="store_true", help='Do not generate list of symbols in fw_img_v1/fw_img_v2 output array but instead generate a C header containing the symbol Ids and addresses.')
    parser.add_argument('--exclude-dummy', dest='exclude_dummy', action="store_true", help='Do not include symbol IDs ending in _DUMMY in the output symbol table C header. Only used when no --sym-input is specified.')
    parser.add_argument('--skip-command-print', dest='skip_command_print', action="store_true", default=False, help='Skip printing command')
    parser.add_argument('--output-directory', dest='output_directory', default=None, help="Output directory of files. By default uses current work dir")

    return parser.parse_args(args[1:])

def validate_args(args):
    # Check that WMFW path exists
    if (not os.path.exists(args.wmfw)):
        print("Invalid wmfw path: " + args.wmfw)
        return False
    if (args.wmdrs is not None):
        # Check that WMDR path(s) exists
        for wmdr in args.wmdrs:
            if (not os.path.exists(wmdr)):
                print("Invalid wmdr path: " + wmdr)
                return False

    # Check that all symbol id header files exist
    if ((args.command == 'fw_img_v1' or args.command == 'fw_img_v2') and (args.symbol_id_input is not None)):
        if (not os.path.exists(args.symbol_id_input)):
            print("Invalid Symbol Header path: " + args.symbol_id_input)
            return False

    # Check that block_size_limit >= 4, <= 4140 and a multiple of 4
    if (args.block_size_limit < 4 or
        args.block_size_limit > 4140 or
        args.block_size_limit % 4):
        print("Invalid block_size_limit: " + str(args.block_size_limit))
        print("Value must be between 4 and 4140 bytes, and a multiple of 4.")
        return False

    return True

def print_start():
    print("")
    print("firmware_converter")
    print("Convert from WMFW/WMDR (\"BIN\") Files to C Header/Source")
    print("SDK version " + print_sdk_version(repo_path + '/sdk_version.h'))

    return

def print_args(args):
    print("")
    print("Command: " + args.command)
    print("Part Number: " + args.part_number)
    print("WMFW Path: " + args.wmfw)
    if (args.wmdrs is not None):
        for wmdr in args.wmdrs:
            print("WMDR Path: " + wmdr)
    if (args.bins is not None):
        for bin in args.bins:
            print("Binary addr/path: " + bin.split(',')[0] + '/' + bin.split(',')[1])

    if (args.suffix):
        print("Suffix: " + args.suffix)
    else:
        print("No suffix")

    if (args.command == 'fw_img_v1' or args.command == 'fw_img_v2'):
        if (args.symbol_id_input is not None):
            print("Input Symbol ID Header: " + args.symbol_id_input)
        else:
            print("Input Symbol ID Header: None")

        if (args.symbol_id_output is not None):
            print("Output Symbol ID Header: " + args.symbol_id_output)

    if (args.revision_check):
        print("WMDR FW Revision Check enabled")

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

    # Create address resolver
    res = address_resolver(args.part_number)

    # validate arguments
    print_args(args)
    if (not (validate_args(args))):
        error_exit("Invalid Arguments")

    if (args.wmdrs is not None):
        process_wmdr = True
    else:
        process_wmdr = False
    if (args.bins is not None):
        process_bins = True
    else:
        process_bins = False

    # Parse WMFW and WMDR files
    wmfw = wmfw_parser(args.wmfw)
    wmfw.parse()

    wmdrs = []
    if (process_wmdr):
        for wmdr_filename in args.wmdrs:
            wmdr = wmdr_parser(wmdr_filename)
            wmdr.parse()
            wmdrs.append(wmdr)
    bins = []
    if (process_bins):
        for bin in args.bins:
            if bin.lower().startswith('0x'):
                bin = {'addr': int(bin.split(',')[0], 16), 'path': bin.split(',')[1]}
            else:
                bin = {'addr': int(bin.split(',')[0], 10), 'path': bin.split(',')[1]}
            bin_parsed = bin_parser(bin, res.unresolve(bin['addr']))
            bin_parsed.parse()
            bins.append(bin_parsed)



    suffix = ""
    if (args.suffix):
        suffix = "_" + args.suffix

    # If requested, check WMDR-WMFW compatibility
    if ((args.revision_check) and (process_wmdr)):
        incompatible_wmdrs = []
        wmfw_fw_revision_str = hex(wmfw.fw_id_block.fields['firmware_revision'])
        for wmdr in wmdrs:
            wmdr_fw_revision_str = hex(wmdr.header.fields['firmware_revision'])
            if (wmdr_fw_revision_str != wmfw_fw_revision_str):
                incompatible_wmdrs.append((wmdr.filename, wmdr_fw_revision_str))

        if (len(incompatible_wmdrs) > 0):
            error_str = 'WMDR Incompatible with WMFW!\n'
            error_str += 'WMFW FW Revision: ' + wmfw_fw_revision_str + '\n'
            for w in incompatible_wmdrs:
                error_str += 'WMDR \'' + w[0] + '\' FW Revision: ' + w[1] + '\n'
            error_exit(error_str)

    # Create firmware data blocks - size according to 'block_size_limit'
    fw_data_block_list = fw_block_list(wmfw.get_data_blocks(), args.block_size_limit, res)
    fw_data_block_list.rehash_blocks()

    # Create coeff data blocks - size according to 'block_size_limit'
    coeff_data_block_lists = []
    if (process_wmdr):
        for wmdr in wmdrs:
            coeff_data_block_list = coeff_block_list(wmdr.data_blocks,
                                                     args.block_size_limit,
                                                     res,
                                                     wmfw.fw_id_block)
            coeff_data_block_list.rehash_blocks()
            coeff_data_block_lists.append(coeff_data_block_list)
    # Create bin data blocks - size according to 'block_size_limit'
    bin_data_block_lists = []
    if (process_bins):
        for bin_parsed in bins:
            bin_data_block_list = bin_block_list(bin_parsed.data_blocks,
                                                 args.block_size_limit,
                                                 res)
            bin_data_block_list.rehash_blocks()
            bin_data_block_lists.append(bin_data_block_list)

    # Create firmware exporter factory
    attributes = dict()
    attributes['part_number_str'] = args.part_number
    attributes['fw_meta'] = dict(fw_id = wmfw.fw_id_block.fields['firmware_id'], fw_rev = wmfw.fw_id_block.fields['firmware_revision'])
    attributes['fw_img_version'] = args.fw_img_version
    attributes['suffix'] = suffix
    attributes['symbol_id_input'] = args.symbol_id_input
    attributes['i2c_address'] = args.i2c_address
    attributes['binary_output'] = args.binary_output
    attributes['exclude_wmfw'] = args.exclude_wmfw
    attributes['symbol_id_output'] = args.symbol_id_output
    attributes['max_block_size'] = args.block_size_limit
    attributes['sym_partition'] = args.sym_partition
    attributes['no_sym_table'] = args.no_sym_table
    attributes['exclude_dummy'] = args.exclude_dummy
    attributes['output_directory'] = args.output_directory

    f = firmware_exporter_factory(attributes)

    # Based on command, add firmware exporters
    if (args.command == 'export'):
        f.add_firmware_exporter('c_array')
    elif (args.command == 'fw_img_v1'):
        f.add_firmware_exporter('fw_img_v1')
        if args.no_sym_table:
            f.add_firmware_exporter('c_array')
    elif (args.command == 'fw_img_v2'):
        f.add_firmware_exporter('fw_img_v2')
        if args.no_sym_table:
            f.add_firmware_exporter('c_array')
    elif (args.command == 'wisce'):
        f.add_firmware_exporter('wisce')
    elif (args.command == 'json'):
        f.add_firmware_exporter('json')

    # Update block info based on any WMDR present
    coeff_data_block_list_lengths = []
    bin_data_block_list_lengths = []
    if process_wmdr:
        for coeff_data_block_list in coeff_data_block_lists:
            coeff_data_block_list_lengths.append(len(coeff_data_block_list.blocks))
    if process_bins:
        for bin_data_block_list in bin_data_block_lists:
            bin_data_block_list_lengths.append(len(bin_data_block_list.blocks))
    f.update_block_info(len(fw_data_block_list.blocks), coeff_data_block_list_lengths, bin_data_block_list_lengths)

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

            # If generic_sym CLI argument specified and this is the 'general' algorith, replace the algorithm name
            if ((args.generic_sym) and (alg_block.fields['algorithm_id'] == wmfw.fw_id_block.fields['firmware_id'])):
                algorithm_name = "FIRMWARE"
            else:
                algorithm_name = alg_block.fields['algorithm_name']

            # If this is the 'struct_t' control, it's full name doesn't have the algo name, so add it
            if ('_struct_t' in coeff_desc.fields['coefficient_name']):
                control_name = algorithm_name + "_" + coeff_desc.fields['coefficient_name']
            else:
                control_name = coeff_desc.fields['full_coefficient_name'].replace(alg_block.fields['algorithm_name'], algorithm_name)

            # Add control
            f.add_control(algorithm_name,
                          alg_block.fields['algorithm_id'],
                          control_name,
                          temp_coeff_address)

    # Add metadata text
    metadata_text_lines = []
    metadata_text_lines.append('firmware_converter.py SDK version: ' + print_sdk_version(repo_path + '/sdk_version.h'))
    temp_line = ''
    for arg in argv:
        temp_line = temp_line + ' ' + arg
    if not args.skip_command_print:
        metadata_text_lines.append('Command: ' + temp_line)

    for wmdr in wmdrs:
        if (len(wmdr.informational_text_blocks) > 0):
            metadata_text_lines.append('WMDR Filename: ' + wmdr.filename)
            metadata_text_lines.append('    Informational Text:')
            for block in wmdr.informational_text_blocks:
                for line in block.text.splitlines():
                    metadata_text_lines.append('    ' + line)
            metadata_text_lines.append('')
    for bin in bins:
        metadata_text_lines.append('Binary Filename: ' + bin.filename)
        metadata_text_lines.append('Binary Addres: ' + hex(bin.address))

    for line in metadata_text_lines:
        f.add_metadata_text_line(line)

    # Add FW Blocks
    for block in fw_data_block_list.blocks:
        block_bytes = []
        for byte_str in block[1]:
            block_bytes.append(int.from_bytes(byte_str, 'little', signed=False))

        f.add_fw_block(block[0], block_bytes)

    # Add Coeff Blocks
    if (process_wmdr):
        coeff_block_list_count = 0
        for coeff_data_block_list in coeff_data_block_lists:
            for block in coeff_data_block_list.blocks:
                # Convert from list of bytestring to list of int
                block_bytes = []
                for byte_str in block[1]:
                    block_bytes.append(int.from_bytes(byte_str, 'little', signed=False))

                f.add_coeff_block(coeff_block_list_count, block[0], block_bytes)

            coeff_block_list_count = coeff_block_list_count + 1

    # Add BIN Blocks
    if (process_bins):
        bin_block_list_count = 0
        for bin_data_block_list in bin_data_block_lists:
            for block in bin_data_block_list.blocks:
                block_bytes = []
                for byte_str in block[1]:
                    block_bytes.append(int.from_bytes(byte_str, 'little', signed=False))

                f.add_bin_block(bin_block_list_count, block[0], block_bytes)
            bin_block_list_count += 1

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
        if (process_bins):
            bin_count = 0
            for bin in bins:
                results_str = results_str + 'BIN File: ' + bin['path'] + '\n'
                # results_str = results_str + str(wmdr)
                bin_count = bin_count + 1
    else:
        results_str = f.to_file()

    print_results(results_str)

    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
