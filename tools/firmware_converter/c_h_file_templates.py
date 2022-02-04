#==========================================================================
# (c) 2020-2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Templates for C Source and Header files
# File    : c_h_file_templates.py
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
from firmware_exporter import firmware_exporter
import time

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
header_file_template_str = """/**
 * @file {part_number_lc}_firmware.h
 *
 * @brief {part_number_uc} Firmware C Header File
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

#ifndef {part_number_uc}_FIRMWARE_H
#define {part_number_uc}_FIRMWARE_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_FIRMWARE_META
 * @brief Firmware meta data
 *
 * @{
 */
#define {part_number_uc}_FIRMWARE_ID {fw_id}
/** @} */

{include_coeff_0}

{include_bin_0}

/**
 * @defgroup {part_number_uc}_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
{algorithm_defines}
/** @} */

/**
 * @defgroup {part_number_uc}_FIRMWARE_CONTROL_ADDRESSES
 * @brief Firmware control parameter addresses
 *
 * @{
 */
#define {part_number_uc}_FIRMWARE_REVISION 0x2800010

{control_defines}
/** @} */

{fw_blocks_info}

/**********************************************************************************************************************/

#endif // {part_number_uc}_FIRMWARE_H

"""

header_file_template_fw_blocks_info = """/**
 * Total blocks of {part_number_uc} Firmware
 */
#define {part_number_lc}_total_fw_blocks ({total_fw_blocks})

{include_coeff_1}

{include_bin_1}

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Block of HALO P/X/Y Memory contents for either firmware or coefficient download.
 */
#ifndef HALO_BOOT_BLOCK_T_DEFINED
#define HALO_BOOT_BLOCK_T_DEFINED
typedef struct
{
    uint32_t block_size;    ///< Size of block in bytes
    uint32_t address;       ///< Control Port register address at which to begin loading
    const uint8_t *bytes;   ///< Pointer to array of bytes consisting of block payload
} halo_boot_block_t;
#endif

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Firmware memory block metadata
 */
extern const halo_boot_block_t {part_number_lc}_fw_blocks[];

{include_coeff_2}

{include_bin_2}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
"""

header_file_template_coeff_strs = {
    'include_coeff_0': """/**
 * Coefficient payload is included
 */
#define {part_number_uc}_LOAD_COEFFICIENTS
""",
    'include_coeff_1': """/**
 * Total blocks of {part_number_uc} Coefficient {coeff_index} data
 */
#define {part_number_lc}_total_coeff_blocks_{coeff_index} ({total_coeff_blocks})
""",
    'include_coeff_2': """/**
 * Coefficient {coeff_index} memory block metadata
 */
extern const halo_boot_block_t {part_number_lc}_coeff_blocks_{coeff_index}[];
"""
}

header_file_template_bin_strs = {
    'include_bin_0': """/**
 * Binary payload is included
 */
#define {part_number_uc}_LOAD_BINARY
""",
    'include_bin_1': """/**
 * Total blocks of {part_number_uc} Binary {bin_index} data
 */
#define {part_number_lc}_total_bin_blocks_{bin_index} ({total_bin_blocks})
""",
    'include_bin_2': """/**
 * Binary {bin_index} memory block metadata
 */
extern const halo_boot_block_t {part_number_lc}_bin_blocks_{bin_index}[];
"""
}

source_file_template_str = """/**
 * @file {part_number_lc}_firmware.c
 *
 * @brief {part_number_uc} Firmware C Array File
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

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "{part_number_lc}_firmware.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_FIRMWARE_MEMORY_BLOCKS
 * @brief Firmware memory blocks
 * @details
 * Trailing zero bytes may be appended to fill output words
 *
 * @{
 */
{fw_block_arrays}/** @} */

/**
 * Firmware memory block metadata
 */
const halo_boot_block_t {part_number_lc}_fw_blocks[] = {
{fw_boot_block_entries}
};

{include_coeff_0}

{include_bin_0}

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
"""

source_file_template_fw_block_str = """const uint8_t {part_number_lc}_fw_block_{block_index}[] = {
{block_bytes}
};
"""

source_file_template_fw_boot_block_entry_str = """    {
        .address = {block_address},
        .block_size = {block_size},
        .bytes = {part_number_lc}_fw_block_{block_index}
    },"""

source_file_template_coeff_strs = {
    'include_coeff_0': """/**
 * @defgroup {part_number_uc}_COEFFICIENT_{coeff_index}_MEMORY_BLOCKS
 * @brief Coefficient {coeff_index} memory blocks
 * @details
 * Trailing zero bytes may be appended to fill output words
 *
 * @{
 */
{coeff_block_arrays}/** @} */

/**
 * Coefficient {coeff_index} memory block metadata
 */
const halo_boot_block_t {part_number_lc}_coeff_{coeff_index}_blocks[] = {
{coeff_boot_block_entries}
};
"""
}

source_file_template_coeff_block_str = """const uint8_t {part_number_lc}_coeff_{coeff_index}_block_{block_index}[] = {
{block_bytes}
};
"""

source_file_template_coeff_boot_block_entry_str = """    {
        .address = {block_address},
        .block_size = {block_size},
        .bytes = {part_number_lc}_coeff_{coeff_index}_block_{block_index}
    },"""

source_file_template_bin_strs = {
    'include_bin_0': """/**
 * @defgroup {part_number_uc}_BIN_{bin_index}_MEMORY_BLOCKS
 * @brief Binary {bin_index} memory blocks
 * @details
 * Trailing zero bytes may be appended to fill output words
 *
 * @{
 */
{bin_block_arrays}/** @} */

/**
 * Binary {bin_index} memory block metadata
 */
const halo_boot_block_t {part_number_lc}_bin_{bin_index}_blocks[] = {
{bin_boot_block_entries}
};
"""
}

source_file_template_bin_block_str = """const uint8_t {part_number_lc}_bin_{bin_index}_block_{block_index}[] = {
{block_bytes}
};
"""

source_file_template_bin_boot_block_entry_str = """    {
        .address = {block_address},
        .block_size = {block_size},
        .bytes = {part_number_lc}_bin_{bin_index}_block_{block_index}
    },"""

#==========================================================================
# CLASSES
#==========================================================================
class header_file:
    def __init__(self, part_number_str, fw_meta, no_sym_table, exclude_dummy):
        self.template_str = header_file_template_str
        if not no_sym_table:
            self.template_str = self.template_str.replace('{fw_blocks_info}', header_file_template_fw_blocks_info)
        else:
            self.template_str = self.template_str.replace('{fw_blocks_info}', '')
        self.includes_coeff = False
        self.includes_bin = False
        self.output_str = ''
        self.terms = dict()
        self.terms['part_number_lc'] = part_number_str.lower()
        self.terms['part_number_uc'] = part_number_str.upper()
        self.terms['total_fw_blocks'] = ''
        self.terms['total_coeff_blocks'] = []
        self.terms['total_bin_blocks'] = []
        self.terms['algorithm_defines'] = ''
        self.terms['control_defines'] = ''
        self.terms['include_coeff_0'] = ''
        self.terms['include_coeff_1'] = ''
        self.terms['include_bin_0'] = ''
        self.terms['include_bin_1'] = ''
        self.terms['fw_id'] = fw_meta['fw_id']
        self.terms['metadata_text'] = ' *\n'
        self.algorithm_controls = dict()
        self.exclude_dummy = exclude_dummy
        return

    def update_block_info(self, fw_block_total, coeff_block_totals, bin_block_totals):
        self.terms['total_fw_blocks'] = str(fw_block_total)
        if (coeff_block_totals != None):
            self.includes_coeff = True
            for coeff_block_total in coeff_block_totals:
                self.terms['total_coeff_blocks'].append(str(coeff_block_total))

        if (bin_block_totals != None):
            self.includes_bin = True
            for bin_block_total in bin_block_totals:
                self.terms['total_bin_blocks'].append(str(bin_block_total))

        return

    def add_control(self, algorithm_name, control_name, address):
        if (self.algorithm_controls.get(algorithm_name, None) == None):
            self.algorithm_controls[algorithm_name] = []
        self.algorithm_controls[algorithm_name].append((control_name, address))
        return

    def add_metadata_text_line(self, line):
        self.terms['metadata_text'] = self.terms['metadata_text'] + ' * ' + line + '\n'
        return

    def __str__(self):
        output_str = self.template_str

        # Update firmware metadata
        fw_id_str = " 0x" + "{0:X}".format(self.terms['fw_id'])
        output_str = output_str.replace('{fw_id}', fw_id_str)

        if (len(self.algorithm_controls) > 0):
            temp_alg_str = ""
            temp_ctl_str = ""
            for key in self.algorithm_controls.keys():
                temp_alg_str = temp_alg_str + "#define {part_number_uc}_ALGORITHM_" + key.upper() + "\n"

                temp_ctl_str = temp_ctl_str + "//Definitions for " + key.upper() + " Controls\n"
                for control in self.algorithm_controls[key]:
                    if " " + control[0].upper() + " " in temp_ctl_str:
                        print("[WARNING] Duplicate symbol id skipped: " + control[0].upper() + " (" + hex(control[1]) + ")")
                    elif self.exclude_dummy and control[0].upper().endswith("DUMMY"):
                        continue
                    else:
                        temp_ctl_str = temp_ctl_str + "#define " + control[0].upper() + " 0x" + "{0:X}".format(control[1]) + "\n"

                temp_ctl_str = temp_ctl_str + "\n\n"

            self.terms['algorithm_defines'] = temp_alg_str
            output_str = output_str.replace('{algorithm_defines}\n', self.terms['algorithm_defines'])
            self.terms['control_defines'] = temp_ctl_str
            output_str = output_str.replace('{control_defines}\n', self.terms['control_defines'])
        else:
            output_str = output_str.replace('{algorithm_defines}\n', '')
            output_str = output_str.replace('{control_defines}\n', '')

        if (self.includes_coeff):
            output_str = output_str.replace('{include_coeff_0}\n', header_file_template_coeff_strs['include_coeff_0'])
            coeff_index = 0
            include_coeff_1_str = ''
            include_coeff_2_str = ''
            for total in self.terms['total_coeff_blocks']:
                include_coeff_1_str = include_coeff_1_str + header_file_template_coeff_strs['include_coeff_1']
                include_coeff_1_str = include_coeff_1_str.replace("{coeff_index}", str(coeff_index))
                include_coeff_1_str = include_coeff_1_str.replace('{total_coeff_blocks}', total)
                include_coeff_1_str = include_coeff_1_str + '\n'

                include_coeff_2_str = include_coeff_2_str + header_file_template_coeff_strs['include_coeff_2']
                include_coeff_2_str = include_coeff_2_str.replace("{coeff_index}", str(coeff_index))
                include_coeff_2_str = include_coeff_2_str + '\n'

                coeff_index = coeff_index + 1

            output_str = output_str.replace('{include_coeff_1}\n', include_coeff_1_str)
            output_str = output_str.replace('{include_coeff_2}\n', include_coeff_2_str)

        else:
            output_str = output_str.replace('{include_coeff_0}\n', '')
            output_str = output_str.replace('{include_coeff_1}\n', '')
            output_str = output_str.replace('{include_coeff_2}\n', '')

        if (self.includes_bin):
            output_str = output_str.replace('{include_bin_0}\n', header_file_template_bin_strs['include_bin_0'])
            bin_index = 0
            include_bin_1_str = ''
            include_bin_2_str = ''
            for total in self.terms['total_bin_blocks']:
                include_bin_1_str = include_bin_1_str + header_file_template_bin_strs['include_bin_1']
                include_bin_1_str = include_bin_1_str.replace("{bin_index}", str(bin_index))
                include_bin_1_str = include_bin_1_str.replace('{total_bin_blocks}', total)
                include_bin_1_str = include_bin_1_str + '\n'

                include_bin_2_str = include_bin_2_str + header_file_template_bin_strs['include_bin_2']
                include_bin_2_str = include_bin_2_str.replace("{bin_index}", str(bin_index))
                include_bin_2_str = include_bin_2_str + '\n'

                bin_index = bin_index + 1

            output_str = output_str.replace('{include_bin_1}\n', include_bin_1_str)
            output_str = output_str.replace('{include_bin_2}\n', include_bin_2_str)

        else:
            output_str = output_str.replace('{include_bin_0}\n', '')
            output_str = output_str.replace('{include_bin_1}\n', '')
            output_str = output_str.replace('{include_bin_2}\n', '')

        output_str = output_str.replace('{total_fw_blocks}', self.terms['total_fw_blocks'])
        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

class source_file:
    def __init__(self, part_number_str):
        self.template_str = source_file_template_str
        self.includes_coeff = False
        self.includes_bin = False
        self.output_str = ''
        self.terms = dict()
        self.terms['part_number_lc'] = part_number_str.lower()
        self.terms['part_number_uc'] = part_number_str.upper()
        self.terms['fw_block_arrays'] = ''
        self.terms['fw_boot_block_entries'] = ''
        self.total_fw_blocks = 0
        self.total_coeff_blocks = []
        self.total_bin_blocks = []
        self.terms['include_coeff_0'] = ''
        self.terms['coeff_block_arrays'] = []
        self.terms['coeff_boot_block_entries'] = []
        self.terms['bin_block_arrays'] = []
        self.terms['bin_boot_block_entries'] = []
        self.uint8_per_line = 24
        return

    def create_block_string(self, data_bytes):
        temp_data_str = ''
        byte_count = 0
        for data_byte in data_bytes:
            temp_byte_str = "0x" + "{0:0{1}X}".format(data_byte, 2) + ","
            byte_count = byte_count + 1
            if (((byte_count % self.uint8_per_line) == 0) and (byte_count < len(data_bytes))):
                temp_byte_str = temp_byte_str + "\n"
            temp_data_str = temp_data_str + temp_byte_str

        return temp_data_str

    def add_fw_block(self, address, data_bytes):
        # Create string for block data
        temp_str = source_file_template_fw_block_str.replace('{block_index}', str(self.total_fw_blocks))
        temp_str = temp_str.replace('{block_bytes}', self.create_block_string(data_bytes))

        self.terms['fw_block_arrays'] = self.terms['fw_block_arrays'] + temp_str + '\n'

        # Create string for boot block entry
        temp_str = source_file_template_fw_boot_block_entry_str.replace('{block_index}', str(self.total_fw_blocks))
        temp_str = temp_str.replace('{block_address}', "0x" + "{0:0{1}X}".format(address, 8))
        temp_str = temp_str.replace('{block_size}', str(len(data_bytes)))

        self.terms['fw_boot_block_entries'] = self.terms['fw_boot_block_entries'] + temp_str + '\n'

        self.total_fw_blocks = self.total_fw_blocks + 1
        return

    def add_coeff_block(self, index, address, data_bytes):
        self.includes_coeff = True

        # Create list elements if they do not exist
        if (len(self.terms['coeff_block_arrays']) < (index + 1)):
            self.terms['coeff_block_arrays'].append('')
        if (len(self.terms['coeff_boot_block_entries']) < (index + 1)):
            self.terms['coeff_boot_block_entries'].append('')
        if (len(self.total_coeff_blocks) < (index + 1)):
            self.total_coeff_blocks.append(0)

        # Create string for block data
        temp_str = source_file_template_coeff_block_str.replace('{block_index}', str(self.total_coeff_blocks[index]))
        temp_str = temp_str.replace('{block_bytes}', self.create_block_string(data_bytes))
        temp_str = temp_str.replace('{coeff_index}', str(index))

        self.terms['coeff_block_arrays'][index] = self.terms['coeff_block_arrays'][index] + temp_str + '\n'

        # Create string for boot block entry
        temp_str = source_file_template_coeff_boot_block_entry_str.replace('{block_index}', str(self.total_coeff_blocks[index]))
        temp_str = temp_str.replace('{block_address}', "0x" + "{0:0{1}X}".format(address, 8))
        temp_str = temp_str.replace('{block_size}', str(len(data_bytes)))
        temp_str = temp_str.replace('{coeff_index}', str(index))

        self.terms['coeff_boot_block_entries'][index] = self.terms['coeff_boot_block_entries'][index] + temp_str + '\n'

        self.total_coeff_blocks[index] = self.total_coeff_blocks[index] + 1
        return

    def add_bin_block(self, index, address, data_bytes):
        self.includes_bin = True

        # Create list elements if they do not exist
        if (len(self.terms['bin_block_arrays']) < (index + 1)):
            self.terms['bin_block_arrays'].append('')
        if (len(self.terms['bin_boot_block_entries']) < (index + 1)):
            self.terms['bin_boot_block_entries'].append('')
        if (len(self.total_bin_blocks) < (index + 1)):
            self.total_bin_blocks.append(0)

        # Create string for block data
        temp_str = source_file_template_bin_block_str.replace('{block_index}', str(self.total_bin_blocks[index]))
        temp_str = temp_str.replace('{block_bytes}', self.create_block_string(data_bytes))
        temp_str = temp_str.replace('{bin_index}', str(index))

        self.terms['bin_block_arrays'][index] = self.terms['bin_block_arrays'][index] + temp_str + '\n'

        # Create string for boot block entry
        temp_str = source_file_template_bin_boot_block_entry_str.replace('{block_index}', str(self.total_bin_blocks[index]))
        temp_str = temp_str.replace('{block_address}', "0x" + "{0:0{1}X}".format(address, 8))
        temp_str = temp_str.replace('{block_size}', str(len(data_bytes)))
        temp_str = temp_str.replace('{bin_index}', str(index))

        self.terms['bin_boot_block_entries'][index] = self.terms['bin_boot_block_entries'][index] + temp_str + '\n'

        self.total_bin_blocks[index] = self.total_bin_blocks[index] + 1
        return

    def __str__(self):
        output_str = self.template_str
        output_str = output_str.replace('{fw_block_arrays}', self.terms['fw_block_arrays'])
        output_str = output_str.replace('{fw_boot_block_entries}', self.terms['fw_boot_block_entries'])

        if (self.includes_coeff):
            temp_str = ''
            for i in range(0, len(self.terms['coeff_block_arrays'])):
                temp_str = temp_str + source_file_template_coeff_strs['include_coeff_0'].replace('{coeff_index}', str(i))
                temp_str = temp_str.replace('{coeff_block_arrays}', self.terms['coeff_block_arrays'][i])
                temp_str = temp_str.replace('{coeff_boot_block_entries}', self.terms['coeff_boot_block_entries'][i])
                temp_str = temp_str + '\n'
            output_str = output_str.replace('{include_coeff_0}\n', temp_str)
        else:
            output_str = output_str.replace('{include_coeff_0}\n', '')

        if (self.includes_bin):
            temp_str = ''
            for i in range(0, len(self.terms['bin_block_arrays'])):
                temp_str = temp_str + source_file_template_bin_strs['include_bin_0'].replace('{bin_index}', str(i))
                temp_str = temp_str.replace('{bin_block_arrays}', self.terms['bin_block_arrays'][i])
                temp_str = temp_str.replace('{bin_boot_block_entries}', self.terms['bin_boot_block_entries'][i])
                temp_str = temp_str + '\n'
            output_str = output_str.replace('{include_bin_0}\n', temp_str)
        else:
            output_str = output_str.replace('{include_bin_0}\n', '')

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

class source_file_exporter(firmware_exporter):

    def __init__(self, attributes):
        firmware_exporter.__init__(self, attributes)
        self.hf = header_file(self.attributes['part_number_str'], self.attributes['fw_meta'],
                              self.attributes['no_sym_table'], self.attributes['exclude_dummy'])
        self.cf = source_file(self.attributes['part_number_str'])
        self.gen_only_include_file = False
        if self.attributes['no_sym_table']:
            self.gen_only_include_file = True

        return

    def update_block_info(self, fw_block_total, coeff_block_totals, bin_block_totals):
        return self.hf.update_block_info(fw_block_total, coeff_block_totals, bin_block_totals)

    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        return self.hf.add_control(algorithm_name, control_name, address)

    def add_metadata_text_line(self, line):
        return self.hf.add_metadata_text_line(line)

    def add_fw_block(self, address, data_bytes):
        return self.cf.add_fw_block(address, data_bytes)

    def add_coeff_block(self, index, address, data_bytes):
        return self.cf.add_coeff_block(index, address, data_bytes)

    def add_bin_block(self, index, address, data_bytes):
        return self.cf.add_bin_block(index, address, data_bytes)

    def to_file(self):
        results_str = 'Exported to files:\n'

        # Write output to filesystem
        temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_firmware.h"
        if self.attributes['output_directory']:
            if not os.path.exists(self.attributes['output_directory']):
                os.makedirs(self.attributes['output_directory'])
            temp_filename = os.path.join(self.attributes['output_directory'], temp_filename)

        f = open(temp_filename, 'w')
        f.write(str(self.hf))
        f.close()
        results_str = results_str + temp_filename + '\n'

        if not self.gen_only_include_file:
            temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_firmware.c"
            if self.attributes['output_directory']:
                if not os.path.exists(self.attributes['output_directory']):
                    os.makedirs(self.attributes['output_directory'])
                temp_filename = os.path.join(self.attributes['output_directory'], temp_filename)

            f = open(temp_filename, 'w')
            f.write(str(self.cf))
            f.close()
            results_str = results_str + temp_filename + '\n'

        return results_str

    def __str__(self):
        return ''

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

