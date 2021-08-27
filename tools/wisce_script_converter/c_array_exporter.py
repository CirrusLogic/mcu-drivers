# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
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
import time

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
header_file_template_str = """/**
 * @file {filename_prefix_lc}_syscfg_regs.h
 *
 * @brief Register values to be applied after {part_number_uc} Driver boot().
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
{metadata_text} *
 */

#ifndef {filename_prefix_uc}_SYSCFG_REGS_H
#define {filename_prefix_uc}_SYSCFG_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stdint.h"
#include "regmap.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define {filename_prefix_uc}_SYSCFG_REGS_TOTAL ({syscfg_regs_total})

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern uint32_t {filename_prefix_lc}_syscfg_regs[];

#ifdef __cplusplus
}
#endif

#endif // {filename_prefix_uc}_SYSCFG_REGS_H

"""

source_file_template_str = """/**
 * @file {filename_prefix_lc}_syscfg_regs.c
 *
 * @brief Register values to be applied after {part_number_uc} Driver boot().
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
#include "{filename_prefix_lc}_syscfg_regs.h"
#include "{part_number_lc}_spec.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

{syscfg_regs_list}

"""

source_file_template_syscfg_reg_title_entry_str = """uint32_t {filename_prefix_lc}_syscfg_regs[] =\n"""
source_file_template_syscfg_reg_list_entry_str = """    {array}{comma}{comment}"""

# ==========================================================================
# CLASSES
# ==========================================================================
class c_array_exporter(wisce_script_exporter):
    def __init__(self, attributes):
        wisce_script_exporter.__init__(self, attributes)
        self.transaction_list = []
        self.transaction_list_length = 0
        self.terms = dict()
        self.terms['part_number_lc'] = self.attributes['part_number_str'].lower()
        self.terms['part_number_uc'] = self.attributes['part_number_str'].upper()
        self.terms['transaction_list'] = ''
        self.terms['metadata_text'] = ''
        self.output_path = self.attributes['output_path']
        self.include_comments = self.attributes['include_comments']

        self.terms['filename_prefix_lc'] = self.terms['part_number_lc']
        self.terms['filename_prefix_uc'] = self.terms['part_number_uc']
        if (self.attributes['suffix'] is not None):
            self.terms['filename_prefix_lc'] += '_' + self.attributes['suffix'].lower()
            self.terms['filename_prefix_uc'] += '_' + self.attributes['suffix'].upper()

        return

    def add_transaction(self, transaction):
        self.transaction_list.append(transaction)
        if not isinstance(transaction, str):
            self.transaction_list_length += transaction.length
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
        output_str = output_str.replace('{filename_prefix_lc}', self.terms['filename_prefix_lc'])
        output_str = output_str.replace('{filename_prefix_uc}', self.terms['filename_prefix_uc'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def to_string(self, is_header):
        if (is_header):
            output_str = header_file_template_str
        else:
            output_str = source_file_template_str

        if (is_header):
            output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])
        else:
            temp_str = source_file_template_syscfg_reg_title_entry_str
            temp_str += "{\n"
            block_write_warning_flag = False
            for t in self.transaction_list:
                if isinstance(t, str):
                    if self.include_comments:
                        temp_str += '{space}// ' + t + '\n'
                    continue

                temp_str = temp_str.replace('{comma}', ',')

                if "{part_number_uc}_SYM_" in t.params[0]:
                    continue
                temp_str += source_file_template_syscfg_reg_list_entry_str
                if t.cmd == 'write':
                    command = ""
                elif t.cmd == 'block_write':
                    if not block_write_warning_flag:
                        print("[WARNING] c_array block write is currently supported only on little-endian systems")
                        block_write_warning_flag = True
                    command = "REGMAP_ARRAY_BLOCK_WRITE, "
                elif t.cmd == 'rmodw':
                    command = "REGMAP_ARRAY_RMODW, "
                elif (t.cmd == 'wait') or (t.cmd == 'insert_delay_ms') or (t.cmd == 'insert_delay'):
                    if t.cmd == 'insert_delay':
                        t.params = str(int(t.params) * 1000)
                    if self.include_comments:
                        if t.comment is None:
                            t.comment = " (Delay for " + t.params + "ms)"
                        else:
                            t.comment += " (Delay for " + t.params + "ms)"
                    command = "REGMAP_ARRAY_DELAY, "
                temp_str = temp_str.replace('{array}', command + t.params)

                if (self.include_comments and (t.comment is not None)):
                    if not t.comment.endswith('\n'):
                        temp_str = temp_str.replace('{comment}', ' // ' + t.comment + '\n')
                    else:
                        temp_str = temp_str.replace('{comment}', ' // ' + t.comment)
                else:
                    temp_str = temp_str.replace('{comment}', '\n')
            temp_str = temp_str.replace('{comma}', '')# remove last comma
            temp_str += "};\n"
            output_str = output_str.replace('{syscfg_regs_list}', temp_str)

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('{filename_prefix_lc}', self.terms['filename_prefix_lc'])
        output_str = output_str.replace('{filename_prefix_uc}', self.terms['filename_prefix_uc'])
        output_str = output_str.replace('{space}', "    ")
        output_str = output_str.replace('{syscfg_regs_total}', str(self.transaction_list_length))
        output_str = output_str.replace('\n\n\n', '\n\n')
        output_str = output_str.replace('{space}', "    ")
        return output_str

    def to_file(self):
        results_str = "Exported to:\n"

        temp_filename = self.output_path + '/' + self.terms['filename_prefix_lc'] + "_syscfg_regs.h"
        f = open(temp_filename, 'w')
        f.write(self.to_string(True))
        f.close()
        results_str += temp_filename + '\n'

        temp_filename = self.output_path + '/' + self.terms['filename_prefix_lc'] + "_syscfg_regs.c"
        f = open(temp_filename, 'w')
        f.write(self.to_string(False))
        f.close()
        results_str += temp_filename + '\n'

        return results_str

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================

