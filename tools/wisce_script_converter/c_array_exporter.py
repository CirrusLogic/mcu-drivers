# ==========================================================================
# (c) 2020 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Class for exporting to C Array
# File    : c_array_exporter.py
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
from wisce_script_exporter import wisce_script_exporter
from wisce_script_transaction import wisce_script_transaction

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
header_file_template_str = """/**
 * @file {part_number_lc}_syscfg_regs.h
 *
 * @brief Register values to be applied after {part_number_uc} Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
{metadata_text} *
 */

#ifndef {part_number_uc}_SYSCFG_REGS_H
#define {part_number_uc}_SYSCFG_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stdint.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define {part_number_uc}_SYSCFG_REGS_TOTAL    ({syscfg_regs_total})

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef struct
{
    uint32_t address;
    uint32_t mask;
    uint32_t value;
} syscfg_reg_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern const syscfg_reg_t {part_number_lc}_syscfg_regs[];

#ifdef __cplusplus
}
#endif

#endif // {part_number_uc}_SYSCFG_REGS_H

"""

source_file_template_str = """/**
 * @file {part_number_lc}_syscfg_regs.c
 *
 * @brief Register values to be applied after {part_number_uc} Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
#include "{part_number_lc}_syscfg_regs.h"
#include "{part_number_lc}_spec.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
const syscfg_reg_t {part_number_lc}_syscfg_regs[] =
{
{syscfg_regs_list}};

"""

source_file_template_syscfg_reg_list_entry_str = """    {{address}, {mask}, {value}}, {comment}"""

# ==========================================================================
# CLASSES
# ==========================================================================
class c_array_exporter(wisce_script_exporter):
    def __init__(self, attributes):
        wisce_script_exporter.__init__(self, attributes)
        self.transaction_list = []
        self.terms = dict()
        self.terms['part_number_lc'] = self.attributes['part_number_str'].lower()
        self.terms['part_number_uc'] = self.attributes['part_number_str'].upper()
        self.terms['transaction_list'] = ''
        self.terms['metadata_text'] = ''
        self.output_path = self.attributes['output_path']
        self.include_comments = self.attributes['include_comments']
        return

    def add_transaction(self, transaction):
        self.transaction_list.append(transaction)
        return

    def add_metadata_text_line(self, line):
        self.terms['metadata_text'] = self.terms['metadata_text'] + ' * ' + line + '\n'
        return

    def __str__(self):
        output_str = source_file_template_str
        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])
        output_str = output_str.replace('{transaction_list}', self.terms['transaction_list'])
        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def to_string(self, is_header):
        if (is_header):
            output_str = header_file_template_str
        else:
            output_str = source_file_template_str

        if (is_header):
            output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])
            output_str = output_str.replace('{syscfg_regs_total}', str(len(self.transaction_list)))
        else:
            temp_str = ''
            for t in self.transaction_list:
                temp_str += source_file_template_syscfg_reg_list_entry_str
                temp_str = temp_str.replace('{address}', "0x" + "{0:0{1}X}".format(t.address, 8))
                temp_str = temp_str.replace('{mask}', "0x" + "{0:0{1}X}".format(0xFFFFFFFF, 8))
                temp_str = temp_str.replace('{value}', "0x" + "{0:0{1}X}".format(t.value, 8))
                if (self.include_comments and (t.comment is not None)):
                    temp_str = temp_str.replace('{comment}', '// ' + t.comment)
                else:
                    temp_str = temp_str.replace('{comment}', '')
                    temp_str += '\n'
            output_str = output_str.replace('{syscfg_regs_list}', temp_str)

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def to_file(self):
        results_str = "Exported to:\n"

        temp_filename = self.output_path + '/' + self.terms['part_number_lc'] + "_syscfg_regs.h"
        f = open(temp_filename, 'w')
        f.write(self.to_string(True))
        f.close()
        results_str += temp_filename + '\n'

        temp_filename = self.output_path + '/' + self.terms['part_number_lc'] + "_syscfg_regs.c"
        f = open(temp_filename, 'w')
        f.write(self.to_string(False))
        f.close()
        results_str += temp_filename + '\n'

        return results_str

class c_array_parser:
    def __init__(self, filename, part_number):
        self.filename = filename
        self.part_number = part_number

        return

    def parse(self):
        comparison_str = source_file_template_str
        comparison_str = comparison_str.replace('{part_number_lc}', self.part_number.lower())
        comparison_str = comparison_str.replace('{part_number_uc}', self.part_number.upper())
        comparison_str = comparison_str.split('\n')
        comparison_lines = []
        for s in comparison_str:
            comparison_lines.append(s + '\n')

        f = open(self.filename, 'r')
        lines = f.readlines()
        f.close()

        array_lines = None
        for i in range(0, len(lines)):
            if ('{syscfg_regs_list}};' in comparison_lines[i]):
                array_lines = lines[i:-1]
                for j in range(0, len(array_lines)):
                    if '};' in array_lines[j]:
                        array_lines = array_lines[0:j]
                break

            elif comparison_lines[i] != lines[i]:
                return None

        if (array_lines is None):
            return None

        entries = []
        for line in array_lines:
            line = line.replace('{', '').replace('}', '')
            line = line.split(',')
            address = line[0].split()
            mask = line[1].split()
            value = line[2].split()
            has_comment = '//' in line[3]
            entry = (address, mask, value, has_comment)
            entries.append(entry)

        return entries

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================

