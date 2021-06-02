# ==========================================================================
# (c) 2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Class for exporting to C Array
# File    : c_function_exporter.py
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
import time
import os

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
header_file_template_str = """/**
 * @file {part_number_lc}_syscfg_regs.h
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

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * FUNCTIONS
 **********************************************************************************************************************/
uint32_t {part_number_lc}_apply_syscfg({part_number_lc}_t *driver);

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
#include "{part_number_lc}_syscfg_regs.h"
#include "{part_number_lc}_spec.h"

/***********************************************************************************************************************
 * COMMANDS
 **********************************************************************************************************************/
uint32_t {part_number_lc}_apply_syscfg({part_number_lc}_t *driver)
{
{syscfg_cmd_list}
{space}{return}
}

"""

function_template = """{space}{part_number_lc}{command}{params}{comment}"""

# ==========================================================================
# CLASSES
# ==========================================================================
class c_function_exporter(wisce_script_exporter):
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
            output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])
        else:
            output_str = source_file_template_str
            temp_str = ''
            for t in self.transaction_list:
                temp_str += function_template
                if t.cmd == 'comment':
                    temp_str = temp_str.replace('{part_number_lc}', '')
                    temp_str = temp_str.replace('{command}', '')
                    temp_str = temp_str.replace('{params}', '')
                    if self.include_comments and t.comment is not None:
                        temp_str = temp_str.replace('{comment}', '// ' + t.comment)
                    else:
                        temp_str = temp_str.replace('{space}', '')
                        temp_str = temp_str.replace('{comment}', '')
                        continue
                elif t.cmd == 'write':
                    temp_str = temp_str.replace('{command}', '_write_reg')
                    temp_str = temp_str.replace('{params}', "(driver, " + # {part_number_lc}_t *driver
                                                            "0x{:04x}".format(t.params[0]) + ", " + # uint32_t addr
                                                            "0x{:04x}".format(t.params[1]) + ")") # uint32_t val
                elif t.cmd == 'wait' or t.cmd == 'insert_delay_ms':
                    temp_str = temp_str.replace('{command}', '_wait')
                    temp_str = temp_str.replace('{params}', "(" + str(t.params[0]) + ")")
                elif t.cmd == 'insert_delay':
                    temp_str = temp_str.replace('{command}', '_wait')
                    temp_str = temp_str.replace('{params}', "(" + str(t.params[0]*1000) + ")") # change to ms
                elif t.cmd == 'block_write':
                    temp_str = temp_str.replace('{command}', '_write_block')
                    # Cut 32bit values into array of uint8's
                    byte_array = t.params[1]
                    for i in range(0, len(byte_array)):
                        if byte_array[i].endswith('\n'):
                            byte_array[i] = byte_array[i][:-1]
                        if byte_array[i].startswith('0x'):
                            byte_array[i] = byte_array[i][2:]
                        if (len(byte_array[i]) % 2) != 0:
                            byte_array[i] = '0' + byte_array[i]
                        byte_array[i] = [byte_array[i][j:j+2] for j in range(0, len(byte_array[i]), 2)]
                    byte_array = [j for i in byte_array for j in i]
                    byte_array_len = len(byte_array)

                    # Format for printing
                    byte_array = str(['0x' + item for item in byte_array]).replace('\'', '')\
                                                                              .replace('[', '')\
                                                                              .replace(']', '')
                    j = 0
                    for i in range(0, len(byte_array)):
                        if i % 47 == 0 and i != 0:
                            byte_array = byte_array[0:j] + " \\\n" + ("{space}" * 13) + byte_array[j:]
                            j += 95
                        j += 1
                    temp_str = temp_str.replace('{params}', "(driver, " + # {part_number_lc}_t *driver
                                                            "0x{:04x}".format(t.params[0]) + ", " + # uint32_t addr
                                                            "(uint8_t[]){" + byte_array + "} , " + # uint8_t *data
                                                            str(byte_array_len) + ")") #uint32_t size

                elif t.cmd == 'rmodw':
                    temp_str = temp_str.replace('{command}', '_update_reg')
                    temp_str = temp_str.replace('{params}',
                                                "(driver, " +           # {part_number_lc}_t *driver
                                                "0x{:04x}".format(t.params[0]) + ", " +  # uint32_t addr
                                                "0x{:04x}".format(t.params[2]) + ", " +  # uint32_t mask
                                                "0x{:04x}".format(t.params[1]) + ")")    # uint32_t val

                if (self.include_comments and (t.comment is not None)):
                    temp_str = temp_str.replace('{comment}', '; // ' + t.comment)
                    if not t.comment.endswith('\n'):
                        temp_str += '\n'
                else:
                    temp_str = temp_str.replace('{comment}', ';')
                    temp_str += '\n'
                temp_str = temp_str.replace('{part_number_lc}', self.terms['part_number_lc'])
                temp_str = temp_str.replace('{space}', "    ")
            output_str = output_str.replace('{syscfg_cmd_list}', temp_str)

        output_str = output_str.replace('{return}', "return {part_number_uc}_STATUS_OK;")
        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('\n\n\n', '\n\n')
        output_str = output_str.replace('{space}', "    ")
        return output_str

    def to_file(self):
        results_str = "Exported to:\n"

        if not(os.path.exists(self.output_path)) and not(os.path.isdir(self.output_path)):
            os.mkdir(self.output_path)

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

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================

