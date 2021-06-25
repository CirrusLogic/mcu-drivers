# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Import WISCE Script transactions
# File    : wisce_script_importer.py
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
import re
from wisce_script_transaction import wisce_script_transaction
from wisce_script_function import wisce_script_function
from sys import platform

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
wisce_script_keywords_to_skip_array = ['loadbinary',
                                       'reset',
                                       'profile',
                                       'wait',
                                       'if',
                                       'else',
                                       'endif',
                                       'check',
                                       'prompt_to_continue',
                                       'prompt_continue_or_quit',
                                       'exit',
                                       'insert_delay',
                                       'loadfirmware',
                                       'block_write',
                                       'block_read',
                                       'smbus_16inx_16dat',
                                       '*',
                                       'message']
wisce_script_keywords_to_skip_functions = ['loadbinary',
                                           'reset',
                                           'profile',
                                           'if',
                                           'else',
                                           'endif',
                                           'check',
                                           'prompt_to_continue',
                                           'prompt_continue_or_quit',
                                           'exit',
                                           'loadfirmware',
                                           'block_read',
                                           'smbus_16inx_16dat']
wisce_script_transaction_keywords = ['4wirespi_32inx_32dat', '4wirespi_32inx_16dat', 'smbus_32inx_32dat']


# ==========================================================================
# CLASSES
# ==========================================================================
class wisce_script_importer:
    def __init__(self, filename, command, symbol_list=None):
        self.transaction_list = []
        self.command = command
        if isinstance(symbol_list, set):
            self.symbol_list = symbol_list
        else:
            self.prepare_symbol_list(symbol_list)
        if (self.command == 'c_array'):
            wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_array
        elif (self.command == 'c_functions'):
            wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_functions

        f = open(filename.strip())
        iter_lines = iter(f.readlines())
        for line in iter_lines:
            symbol = False
            # Check for starting with a keyword
            line = line.replace('\t', ' ')
            words = line.split(' ')
            words = [w for w in words if w != '']
            raw_words = words
            words = [word.lower() for word in words]
            if (len(words) == 0) or words[0] in wisce_script_keywords_to_skip:
                continue

            # Get any comments
            comment = None
            if (line.find('*') != -1):
                comment = line[line.index('*'):]
            elif (line.find('message') != -1):
                comment = line[line.index('message'):]

            if (self.command == 'c_array'):
                if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
                    # First check that it is a 'Write' transaction
                    if (words[3] == 'write'):
                        addr = self.formatRegister(words[0])
                        value = self.formatRegister(words[1])
                        if len(addr) == 1:
                            mask = "0x" + "{0:0{1}X}".format(0xFFFFFFFF, 8)
                        else:
                            mask = addr[1] + "_MASK"
                            value[0] += " << " + addr[1] + "_SHIFT"
                        self.transaction_list.append(wisce_script_transaction(addr[0],
                                                                              value[0],
                                                                              mask,
                                                                              comment))
                elif (words[0] == 'load'):
                    subfile = line.split(raw_words[0] + ' ')[1]
                    subfile = subfile.replace('\r', '').replace('\n', '').replace('"', '').replace('"', '')
                    if platform == "win32":
                        subfile = subfile.replace('/', os.path.sep)
                    else:
                        subfile = subfile.replace('\\', os.path.sep)
                    path = os.path.dirname(filename)
                    subimporter = wisce_script_importer(os.path.join(path, subfile), command, self.symbol_list)
                    self.transaction_list.extend(subimporter.transaction_list)
            elif (self.command == 'c_functions'):
                if (words[0].find("*") != -1) or words[0] == 'message':
                    self.transaction_list.append(wisce_script_function('comment',
                                                                       '',
                                                                       comment))
                elif (words[0] == 'wait') or (words[0] == 'insert_delay_ms') or (words[0] == 'insert_delay'):
                    self.transaction_list.append(wisce_script_function(words[0],
                                                                       [int(words[1], 10)],
                                                                       comment))
                elif words[0] == 'block_write':
                    data = []
                    line = next(iter_lines)
                    while "end" not in line.lower():
                        data_items = line.split(' ')
                        data_items = [d for d in data_items if d != '']
                        data_items = [d for d in data_items if d != '\n']
                        data.extend(data_items)
                        line = next(iter_lines)

                    # Cut 32bit values into array of uint8's
                    byte_array = data
                    for i in range(0, len(byte_array)):
                        if byte_array[i].endswith('\n'):
                            byte_array[i] = byte_array[i][:-1]
                        if byte_array[i].startswith('0x'):
                            byte_array[i] = byte_array[i][2:]
                        if (len(byte_array[i]) % 2) != 0:
                            byte_array[i] = '0' + byte_array[i]
                        byte_array[i] = [byte_array[i][j:j + 2] for j in range(0, len(byte_array[i]), 2)]
                    byte_array = [j for i in byte_array for j in i]
                    byte_array_len = len(byte_array)

                    # Format for printing
                    byte_array = str(['0x' + item for item in byte_array]).replace('\'', '') \
                        .replace('[', '') \
                        .replace(']', '')
                    j = 0
                    for i in range(0, len(byte_array)):
                        if i % 47 == 0 and i != 0:
                            byte_array = byte_array[0:j] + " \\\n" + ("{space}" * 13) + byte_array[j:]
                            j += 95
                        j += 1
                    byte_array = "(uint8_t[]){" + byte_array + "}"

                    addr = self.formatRegister(words[1])

                    self.transaction_list.append(wisce_script_function(words[0],
                                                                       [addr[0],
                                                                        byte_array,
                                                                        str(byte_array_len)],
                                                                       comment))
                elif (len(words) > 3) and (words[3] == 'write'):
                    if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):

                        addr = self.formatRegister(words[0])

                        if len(addr) == 1:
                            self.transaction_list.append(wisce_script_function(words[3],
                                                                               [addr[0],
                                                                                "0x{:04x}".format(int(words[1], 16))],
                                                                               comment))
                        else:
                            self.transaction_list.append(wisce_script_function('rmodw',
                                                                               [addr[0],
                                                                                "0x{:04x}".format(int(words[1], 16))
                                                                                   + " << " + addr[1] + "_SHIFT",
                                                                                addr[1] + "_MASK"],
                                                                               comment))
                elif (len(words) > 3) and (words[3] == 'rmodw'):
                    if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in
                            words)):
                        value = "0x{:04x}".format(int(words[1], 16))
                        if len(words) >= 6 and (words[5].find("*") == -1) and words[5] != 'message':
                            if all(char in '0123456789abcdefx' for char in words[5]):
                                mask = "0x{:04x}".format(int(words[5], 16))
                            else:
                                mask = "{part_number_uc}_" + words[5].upper() + "_MASK"
                                shift = "{part_number_uc}_" + words[5].upper() + "_SHIFT"
                                value += " << " + shift
                        else:
                            mask = '0xffffffff'

                        addr = self.formatRegister(words[0])

                        self.transaction_list.append(wisce_script_function(words[3],
                                                                           [addr[0],  # addr
                                                                            value,  # val
                                                                            mask],  # mask
                                                                           comment))
                elif (words[0] == 'load'):
                    subfile = line.split(raw_words[0] + ' ')[1]
                    subfile = subfile.replace('\r', '').replace('\n', '').replace('"', '').replace('"', '')
                    if platform == "win32":
                        subfile = subfile.replace('/', os.path.sep)
                    else:
                        subfile = subfile.replace('\\', os.path.sep)
                    path = os.path.dirname(filename)
                    subimporter = wisce_script_importer(os.path.join(path, subfile), command, self.symbol_list)
                    self.transaction_list.extend(subimporter.transaction_list)

        f.close()
        return

    def get_transaction_list(self):
        return self.transaction_list

    def prepare_symbol_list(self, symbol_file):
        self.symbol_list = set()
        if symbol_file is not None:
            f = open(symbol_file.strip())
            iter_lines = iter(f.readlines())
            for line in iter_lines:
                if "_SYM_" in line:
                    words = line.split(' ')
                    words = [w for w in words if w != '']
                    symbol = re.sub(r'CS\d\dL\d\d_SYM_', '', words[1])
                    self.symbol_list.add(symbol)
        return

    def formatRegister(self, toFormat):
        result = []
        if all(char in '0123456789abcdefx' for char in toFormat):
            result.append("0x{:04x}".format(int(toFormat, 16)))
        else:
            if '.' in toFormat:
                toFormat = toFormat.split('.')
                if toFormat[0].upper() in self.symbol_list:
                    toFormat[0] = "SYM_" + toFormat[0]
                result.append("{part_number_uc}_" + toFormat[0].upper())  # reg addr
                result.append("{part_number_uc}_" + toFormat[1].upper())  # mask
            else:
                if toFormat.upper() in self.symbol_list:
                    toFormat = "SYM_" + toFormat
                result.append("{part_number_uc}_" + toFormat.upper())

        return result

    def __str__(self):
        output_str = 'blah'
        return output_str

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
