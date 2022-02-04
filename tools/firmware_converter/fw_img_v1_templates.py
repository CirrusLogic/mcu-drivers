#==========================================================================
# (c) 2020-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Templates for C Source and Header files
# File    : fw_img_v1_templates.py
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
import re
import os
from firmware_exporter import firmware_exporter
from fw_img_v1 import fw_img_v1
from collections import OrderedDict
import time

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
IMG_MAGIC_NUMBER_1 = 0x54b998ff
IMG_MAGIC_NUMBER_2 = 0x936be2a6

header_file_template_str = """/**
 * @file {part_number_lc}_fw_img.h
 *
 * @brief {part_number_uc} FW IMG Header File
 *
 * @copyright
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef {part_number_uc}_FW_IMG_H
#define {part_number_uc}_FW_IMG_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * FW_IMG
 **********************************************************************************************************************/

extern const uint8_t {part_number_lc}_fw_img[];

/**********************************************************************************************************************/

#endif // {part_number_uc}_FW_IMG_H

"""

source_file_template_str = """/**
 * @file {part_number_lc}_fw_img.c
 *
 * @brief {part_number_uc} FW IMG Source File
 *
 * @copyright
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
{metadata_text} *
 */

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "{part_number_lc}_fw_img.h"

/***********************************************************************************************************************
 * FW_IMG
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_FW_IMG
 * @brief Firmware image
 *
 * @{
 */

const uint8_t {part_number_lc}_fw_img[] = {
// Header
{magic_number_1} // IMG_MAGIC_NUMBER_1
{img_format_rev} // IMG_FORMAT_REV
{img_size} // IMG_SIZE
{sym_table_size} // SYM_TABLE_SIZE
{alg_list_size} // ALG_LIST_SIZE
{fw_id} // FW_ID
{fw_ver} // FW_VERSION
{data_block_count} // DATA_BLOCKS
{max_block_size}
{bin_ver}
// Symbol Linking Table
{sym_table}
// Algorithm ID List
{alg_list}
// Firmware Data
{fw_data_blocks}
{coeff_data}
{bin_data}

// Footer
{magic_number_2} // IMG_MAGIC_NUMBER_2
{img_checksum} // IMG_CHECKSUM
};

/** @} */

/**********************************************************************************************************************/

"""

source_file_template_fw_block_str = """{fw_block_size} // FW_BLOCK_SIZE_{block_index}
{fw_block_addr} // FW_BLOCK_ADDR_{block_index}
{block_bytes}"""

source_file_template_bin_block_str = """{bin_block_size} // BIN_BLOCK_SIZE_{block_index}
{bin_block_addr} // BIN_BLOCK_ADDR_{block_index}
{block_bytes}"""

source_file_template_coeff_strs = {
    'include_coeff_0': """// COEFF_DATA_BLOCKS_{coeff_index}
{coeff_block_arrays}"""
}

source_file_template_coeff_block_str = """{coeff_block_size} // COEFF_BLOCK_SIZE_{coeff_index}_{block_index}
{coeff_block_addr} // COEFF_BLOCK_ADDR_{coeff_index}_{block_index}
{block_bytes}"""

symbol_id_header_file_template_str = """
/**
 * @file {part_number_lc}_sym.h
 *
 * @brief Master table of known firmware symbols for the {part_number_uc} Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
{metadata_text} *
 *
 */

#ifndef {part_number_uc}_SYM_H
#define {part_number_uc}_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
{algorithm_defines}
/** @} */

/**
 * @defgroup {part_number_uc}_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
{symbol_id_define_list}

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // {part_number_uc}_SYM_H

"""

#==========================================================================
# CLASSES
#==========================================================================
class fw_img_v1_file(firmware_exporter):
    def __init__(self, attributes, version):
        firmware_exporter.__init__(self, attributes)
        self.includes_coeff = False
        self.includes_bin = False
        self.output_str = ''
        self.terms = dict()
        self.terms['part_uc'] = self.attributes['part_number_str'].upper()
        self.terms['part_lc'] = self.attributes['part_number_str'].lower()
        self.terms['part_number_lc'] = (self.attributes['part_number_str'] + self.attributes['suffix']).lower()
        self.terms['part_number_uc'] = (self.attributes['part_number_str'] + self.attributes['suffix']).upper()
        self.terms['include_coeff_0'] = ''
        self.terms['metadata_text'] = ' *\n'
        self.fw_data_block_list = []
        self.coeff_data_block_list = []
        self.bin_data_block_list = []
        self.uint8_per_line = 24
        self.blocks = []
        self.sym_id_input = self.attributes['symbol_id_input']
        self.sym_id_output = self.attributes['symbol_id_output']

        self.terms['version'] = version
        self.terms['img_size'] = 0
        self.terms['fw_id'] = self.attributes['fw_meta']['fw_id']
        self.terms['fw_rev'] = self.attributes['fw_meta']['fw_rev']
        self.terms['bin_ver'] = self.attributes['fw_img_version']
        self.terms['max_block_size'] = self.attributes['max_block_size']
        self.terms['sym_partition'] = self.attributes['sym_partition']
        self.terms['no_sym_table'] = self.attributes['no_sym_table']
        self.terms['exclude_dummy'] = self.attributes['exclude_dummy']

        self.algorithms = OrderedDict()
        self.algorithm_controls = OrderedDict()

        self.c0 = 0x0
        self.c1 = 0x0

        self.image_word_list = []

        return

    def get_bytes_string(self, data_bytes):
        temp_data_str = ''
        byte_count = 0
        for data_byte in data_bytes:
            temp_byte_str = "0x" + "{0:0{1}X}".format(data_byte, 2) + ","

            byte_count += 1
            if (((byte_count % self.uint8_per_line) == 0) and (byte_count < len(data_bytes))):
                temp_byte_str = temp_byte_str + "\n"
            temp_data_str = temp_data_str + temp_byte_str

        return temp_data_str

    def add_bytes_to_img(self, data_bytes):
        byte_count = 0
        temp_data_word = 0
        for data_byte in data_bytes:
            temp_data_word |= data_byte << ((byte_count % 4) * 8)

            byte_count += 1

            if ((byte_count % 4) == 0):
                self.image_word_list.append(temp_data_word)
                temp_data_word = 0

        self.terms['img_size'] += byte_count
        return self.get_bytes_string(data_bytes)

    def add_word_to_img(self, val):
        return self.add_bytes_to_img(val.to_bytes(4, byteorder='little'))

    def get_word_string(self, val):
        return self.get_bytes_string(val.to_bytes(4, byteorder='little'))

    def update_block_info(self, fw_block_total, coeff_block_totals, bin_block_totals): pass

    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        if (not self.attributes['exclude_wmfw']):
            if self.algorithms.get(algorithm_name, None) is None:
                self.algorithms[algorithm_name] = algorithm_id
                self.algorithm_controls[algorithm_name] = []
            self.algorithm_controls[algorithm_name].append((control_name, address))

    def add_fw_block(self, address, data_bytes):
        if (not self.attributes['exclude_wmfw']):
            # Create entry into data block list
            self.fw_data_block_list.append((len(data_bytes), address, data_bytes))

    def add_coeff_block(self, index, address, data_bytes):
        self.includes_coeff = True

        # Create entry into data block list
        if (len(self.coeff_data_block_list) < (index + 1)):
            self.coeff_data_block_list.append([])

        self.coeff_data_block_list[index].append((len(data_bytes), address, data_bytes))

    def add_bin_block(self, index, address, data_bytes):
        self.includes_bin = True
        # Create entry into data block list
        if (len(self.bin_data_block_list) < (index + 1)):
            self.bin_data_block_list.append([])

        self.bin_data_block_list[index].append((len(data_bytes), address, data_bytes))

    def add_metadata_text_line(self, line):
        self.terms['metadata_text'] = self.terms['metadata_text'] + ' * ' + line + '\n'
        return

    def find_symbol_id(self, sym_name):
        matches = re.findall(r"^#define " + self.terms['part_number_uc'] + "_SYM_" + sym_name.upper() + " .*", self.sym_header, re.M)
        if len(matches) == 1:
            return int(re.sub(r"\).*$", "", re.sub(r"^.*\(", "", matches[0])), base=16)
        return 0

    def generate_symbol_header(self, symbol_id_list):
        output_str = symbol_id_header_file_template_str

        # Iterate through algorithms and respective lists of controls and generate symbol id define list
        temp_str = ''
        # the count for control ids starts at 1 - the value 0 is a special value used by parsing library
        control_id = 1
        control_id_outer = 0

        alg_list_str = ''
        if self.algorithms:
            # First get all control strings and calculate the longest name
            longest_str_len = 0
            for alg_name, alg_id in self.algorithms.items():
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = self.terms['part_number_uc'] + "_SYM_" + control[0].upper()

                    if ((symbol_id_list is None) or
                        ((symbol_id_list is not None) and (temp_ctl_str in symbol_id_list))):
                        temp_ctl_str = "#define " + temp_ctl_str
                        if (len(temp_ctl_str) > longest_str_len):
                            longest_str_len = len(temp_ctl_str)

            # Round up to nearest tab/4 spaces
            longest_str_len += (4 - (longest_str_len % 4))

            # Create string of all symbol id defines
            for alg_name, alg_id in self.algorithms.items():
                if (self.terms['sym_partition']):
                    control_id = (0x10000 * control_id_outer) + 1
                temp_alg_str = ""
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = self.terms['part_number_uc'] + "_SYM_" + control[0].upper()
                    if " " + temp_ctl_str + " " in temp_alg_str:
                        print("[WARNING] Duplicate symbol id skipped: " + str(temp_ctl_str) + " (" + hex(control_id) + ")")
                    elif self.terms['exclude_dummy'] and temp_ctl_str.endswith("DUMMY"):
                        continue
                    else:
                        if ((symbol_id_list is None) or
                            ((symbol_id_list is not None) and (temp_ctl_str in symbol_id_list))):
                            temp_ctl_str = "#define " + temp_ctl_str
                            temp_ctl_str += ' ' * (longest_str_len - len(temp_ctl_str))
                            temp_ctl_str += '(' + hex(control_id) + ')\n'

                            temp_alg_str += temp_ctl_str

                            control_id += 1
                control_id_outer += 1
                if (temp_alg_str != ""):
                    alg_list_str += "#define {part_number_uc}_ALGORITHM_" + alg_name.upper() + "\n"
                    temp_str += "// " + alg_name + "\n" + temp_alg_str

        # Replace algorithm list
        output_str = output_str.replace('{algorithm_defines}\n', alg_list_str)

        # Replace symbol id define list
        output_str = output_str.replace('{symbol_id_define_list}\n', temp_str)

        # Replace metadata
        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        # Replace part number strings
        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('\n\n\n', '\n\n')

        if (self.sym_id_input is None):
            if (self.sym_id_output is None):
                self.sym_id_output = self.terms['part_number_lc'] + '_sym.h'
            f = open(self.sym_id_output, 'w')
            f.write(output_str)
            f.close()

            print("Generated Symbol ID Header " + self.sym_id_output)

        return output_str

    def extract_symbol_ids(self, filename):
        symbol_id_list = []
        f = open(filename, 'r')
        lines = f.readlines()
        for line in lines:
            words = line.split()
            if (len(words) == 3):
                if ((words[0] == '#define') and ('_SYM_' in words[1]) and (words[1] not in symbol_id_list)):
                    symbol_id_list.append(words[1])
        f.close()

        return symbol_id_list

    def fletch32(self, word):
        modval = pow(2, 16) - 1
        bytes = word.to_bytes(4, byteorder='little')
        self.c0 = (self.c0 + (bytes[0] + (bytes[1] << 8))) % modval
        self.c1 = (self.c1 + self.c0) % modval
        self.c0 = (self.c0 + (bytes[2] + (bytes[3] << 8))) % modval
        self.c1 = (self.c1 + self.c0) % modval

    def calc_checksum(self):
        for i in range(0, len(self.image_word_list)):
            self.fletch32(self.image_word_list[i])

        return self.c0 + (self.c1 << 16)

    def to_byte_array(self):
        # Convert to byte array
        byte_list = []
        for w in self.image_word_list:
            byte_list.extend(w.to_bytes(4, byteorder="little", signed=False))

        return bytearray(byte_list)

    def __str__(self):
        return ''

    def create_source_file_text(self):
        output_str = source_file_template_str

        # Update IMG_MAGIC_NUMBER_1 and IMG_FORMAT_REV
        output_str = output_str.replace('{magic_number_1}', self.add_word_to_img(IMG_MAGIC_NUMBER_1))
        output_str = output_str.replace('{img_format_rev}', self.add_word_to_img(self.terms['version']))

        # Skipping IMG_SIZE - added after entire image processed

        # Update SYM_TABLE_SIZE and ALG_LIST_SIZE and
        control_count = 0
        if not self.terms['no_sym_table']:
            if (self.algorithms):
                for alg_name, alg_id in self.algorithms.items():
                    for control in self.algorithm_controls[alg_name]:
                        sym_id = self.find_symbol_id(control[0])
                        if sym_id:
                            control_count += 1
        output_str = output_str.replace('{sym_table_size}', self.add_word_to_img(control_count))
        output_str = output_str.replace('{alg_list_size}', self.add_word_to_img(len(self.algorithms)))

        # Update FW_ID and FW_VERSION
        output_str = output_str.replace('{fw_id}', self.add_word_to_img(self.terms['fw_id']))
        output_str = output_str.replace('{fw_ver}', self.add_word_to_img(self.terms['fw_rev']))

        # Calculate 'DATA_BLOCKS' - number of total data blocks
        data_block_count = len(self.fw_data_block_list)
        for i in range(0, len(self.coeff_data_block_list)):
            data_block_count += len(self.coeff_data_block_list[i])
        for i in range(0, len(self.bin_data_block_list)):
            data_block_count += len(self.bin_data_block_list[i])
        output_str = output_str.replace('{data_block_count}', self.add_word_to_img(data_block_count))

        # Set MAX_BLOCK_SIZE and FW_IMG_VERSION
        if self.terms['version'] == 1:
            output_str = output_str.replace('{max_block_size}', "")
            output_str = output_str.replace('{bin_ver}', "")
        else:
            output_str = output_str.replace('{max_block_size}', self.add_word_to_img(self.terms['max_block_size']) + " // MAX_BLOCK_SIZE")
            output_str = output_str.replace('{bin_ver}', self.add_word_to_img(self.terms['bin_ver']) + " // FW_IMG_VERSION")

        # Add Symbol Linking Table
        if not self.terms['no_sym_table']:
            temp_ctl_str = ''
            if self.algorithms:
                for alg_name, alg_id in self.algorithms.items():
                    for control in self.algorithm_controls[alg_name]:
                        sym_id = self.find_symbol_id(control[0])
                        if sym_id:
                            temp_ctl_str = temp_ctl_str + self.add_word_to_img(sym_id) + " // " + control[0].upper() + "\n" \
                                + self.add_word_to_img(control[1]) + " // " + hex(control[1]) + "\n"

            output_str = output_str.replace('{sym_table}\n', temp_ctl_str)
        else:
            output_str = output_str.replace('{sym_table}\n', "")


        # Add Algorithm ID List
        temp_alg_str = ''
        if self.algorithms:
            for alg_name, alg_id in self.algorithms.items():
                temp_alg_str = temp_alg_str + self.add_word_to_img(alg_id) + " // " + alg_name + "\n"

        output_str = output_str.replace('{alg_list}\n', temp_alg_str)

        # Add Firmware Data
        fw_block_str = ''
        for i in range(0, len(self.fw_data_block_list)):
            address = self.fw_data_block_list[i][1]
            data_bytes = self.fw_data_block_list[i][2]
            # Create string for block data
            temp_str = source_file_template_fw_block_str.replace('{block_index}', str(i))
            temp_str = temp_str.replace('{fw_block_size}', self.add_word_to_img(len(data_bytes)))
            temp_str = temp_str.replace('{fw_block_addr}', self.add_word_to_img(address))
            temp_str = temp_str.replace('{block_bytes}', self.add_bytes_to_img(data_bytes))

            fw_block_str += temp_str + '\n'

        output_str = output_str.replace('{fw_data_blocks}', fw_block_str)

        # Add COEFF_DATA_BLOCKS_
        if (self.includes_coeff):
            temp_str = ''

            for i in range(0, len(self.coeff_data_block_list)):
                temp_str = temp_str + source_file_template_coeff_strs['include_coeff_0'].replace('{coeff_index}', str(i))
                temp_temp_str = ''
                for j in range(0, len(self.coeff_data_block_list[i])):
                    # Create string for block data
                    address = self.coeff_data_block_list[i][j][1]
                    data_bytes = self.coeff_data_block_list[i][j][2]
                    temp_temp_str += source_file_template_coeff_block_str.replace('{block_index}', str(j)) + '\n'
                    temp_temp_str = temp_temp_str.replace('{coeff_index}', str(i))
                    temp_temp_str = temp_temp_str.replace('{coeff_block_size}', self.add_word_to_img(len(data_bytes)))
                    temp_temp_str = temp_temp_str.replace('{coeff_block_addr}', self.add_word_to_img(address))
                    temp_temp_str = temp_temp_str.replace('{block_bytes}', self.add_bytes_to_img(data_bytes))
                temp_str = temp_str.replace('{coeff_block_arrays}', temp_temp_str)
                temp_str = temp_str + '\n'

            output_str = output_str.replace('{coeff_data}\n', temp_str)
        else:
            output_str = output_str.replace('{coeff_data}\n', '')

        # Add arbitrary binary data
        if (self.includes_bin):
            bin_block_str = ''
            for i in range(0, len(self.bin_data_block_list)):
                for j in range(0, len(self.bin_data_block_list[i])):
                    address = self.bin_data_block_list[i][j][1]
                    data_bytes = self.bin_data_block_list[i][j][2]
                    # Create string for block data
                    temp_str = source_file_template_bin_block_str.replace('{block_index}', str(j))
                    temp_str = temp_str.replace('{bin_block_size}', self.add_word_to_img(len(data_bytes)))
                    temp_str = temp_str.replace('{bin_block_addr}', self.add_word_to_img(address))
                    temp_str = temp_str.replace('{block_bytes}', self.add_bytes_to_img(data_bytes))

                    bin_block_str += temp_str + '\n'

            output_str = output_str.replace('{bin_data}\n', bin_block_str)
        else:
            output_str = output_str.replace('{bin_data}\n', '')

        # Update IMG_MAGIC_NUMBER_2
        output_str = output_str.replace('{magic_number_2}', self.add_word_to_img(IMG_MAGIC_NUMBER_2))

        # Update IMG_SIZE
        # need to add 8 to include the checksum and the img_size field itself.
        self.terms['img_size'] += 8
        output_str = output_str.replace('{img_size}', self.get_word_string(self.terms['img_size']))
        self.image_word_list.insert(2, self.terms['img_size'])

        # Calculate IMG_CHECKSUM
        if self.terms['version'] == 1:
            output_str = output_str.replace('{img_checksum}', self.add_word_to_img(0xf0f00f0f))
        else:
            output_str = output_str.replace('{img_checksum}', self.add_word_to_img(self.calc_checksum()))

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def create_header_file_text(self):
        output_str = header_file_template_str

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def to_file(self):
        results_str = 'Exported to files:\n'

        temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_fw_img"
        if self.attributes['output_directory']:
            if not os.path.exists(self.attributes['output_directory']):
                os.makedirs(self.attributes['output_directory'])
            temp_filename = os.path.join(self.attributes['output_directory'], temp_filename)

        # Open or generate the symbol id header
        symbol_id_list = None
        if (self.sym_id_input is not None):
            symbol_id_list = self.extract_symbol_ids(self.sym_id_input)
            tmp = open(self.sym_id_input, 'r')
            self.sym_header = tmp.read()
            tmp.close()
        elif not self.terms['no_sym_table']:
            self.sym_header = self.generate_symbol_header(symbol_id_list)

        if (not self.attributes['binary_output']):
            self.template_str = header_file_template_str
            f = open(temp_filename + ".h", 'w')
            f.write(self.create_header_file_text())
            f.close()
            results_str = results_str + temp_filename + '.h\n'

            self.template_str = source_file_template_str
            f = open(temp_filename + ".c", 'w')
            f.write(self.create_source_file_text())
            f.close()
            results_str = results_str + temp_filename + '.c\n'

        else:
            # Create fw_img contents
            self.create_source_file_text()
            temp_filename = temp_filename + ".bin"
            f = open(temp_filename, "wb")
            f.write(self.to_byte_array())
            f.close()
            results_str = results_str + temp_filename + '\n'

        return results_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

