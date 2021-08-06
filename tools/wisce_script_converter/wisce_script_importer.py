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
from wisce_script_function import wisce_script_function
from sys import platform

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
wisce_script_keywords_to_skip_array = ['loadbinary',
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

        f = open(filename.strip())
        iter_lines = iter(f.readlines())

        if (self.command == 'c_array'):
            self.c_array_import(iter_lines, filename)
        elif (self.command == 'c_functions'):
            self.c_functions_import(iter_lines, filename)

        f.close()
        return

    def c_array_import(self, iter_lines, filename):
        wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_array
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

            # First check that it is a 'Write' transaction
            if (len(words)>3) and (words[3] == 'write'):
                if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
                    addr = self.format_register(words[0])
                    value = self.format_register(words[1])
                    if len(addr) > 1:
                        mask = addr[1] + "_MASK"
                        value[0] = "(" + value[0] + " | " + mask + ") << " + addr[1] + "_SHIFT"
                    self.transaction_list.append(wisce_script_function(words[3],
                                                                       addr[0] + ', ' +
                                                                       value[0],
                                                                       comment,
                                                                       2))
            elif (words[0] == 'block_write'):
                if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
                    data = []
                    line = next(iter_lines)
                    while "end" not in line.lower():
                        data_items = line.split(' ')
                        data_items = [d for d in data_items if d != '']
                        data_items = [d.replace('\n', '').replace('0x', '') for d in data_items if d != '\n']
                        data.extend(data_items)
                        line = next(iter_lines)

                    # if values are not 32bit long, reinterpret them as such
                    if any([len(d) != 8 for d in data]):
                        squashed = ''.join(data)
                        if (len(squashed) % 8) != 0:
                            squashed += '0' * (8 - (len(squashed) % 8)) # pad if not cleanly divisible in 32bit vals
                        data = [(squashed[i:i + 8]) for i in range(0, len(squashed), 8)]
                    data_array_len = len(data)

                    # Format for printing
                    for i in range(0, len(data)):
                        # Swap for casting into uint8_ts
                        data[i] = data[i][6:8] + data[i][4:6] + data[i][2:4] + data[i][0:2]
                    data = str(['0x' + item for item in data]).replace('\'', '') \
                        .replace('[', '') \
                        .replace(']', '')

                    addr = self.format_register(words[1])

                    params = addr[0] + ', ' + '0x' + str(data_array_len) + ', ' + data
                    sep_location = 80
                    while sep_location < len(params):
                        if params[sep_location] is ',':
                            sep_location += 2
                            params = params[0:sep_location] + "\\\n" + "{space}" + params[sep_location:]
                            sep_location += 110
                        sep_location += 1

                    self.transaction_list.append(wisce_script_function(words[0],
                                                                       params,
                                                                       comment,
                                                                       data_array_len + 3))
            elif (len(words) > 3) and (words[3] == 'rmodw'):
                if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
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

                        addr = self.format_register(words[0])

                        self.transaction_list.append(wisce_script_function(words[3],
                                                                           addr[0] + ', ' +
                                                                           value + ', ' +
                                                                           mask,  # mask
                                                                           comment,
                                                                           4))
            elif (words[0] == 'wait') or (words[0] == 'insert_delay_ms') or (words[0] == 'insert_delay'):
                self.transaction_list.append(wisce_script_function(words[0],
                                                                   str(int(words[1], 10)),
                                                                   comment,
                                                                   2))
            elif (words[0] == 'load'):
                self.load_subfile(line, raw_words, filename)

    def c_functions_import(self, iter_lines, filename):
        wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_functions
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

            if (words[0].find("*") != -1) or words[0] == 'message':
                if comment.endswith('\n'):
                    comment = comment[:-1]
                self.transaction_list.append(comment)
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

                addr = self.format_register(words[1])

                self.transaction_list.append(wisce_script_function(words[0],
                                                                   [addr[0],
                                                                    byte_array,
                                                                    str(byte_array_len)],
                                                                   comment))
            elif (len(words) > 3) and (words[3] == 'write'):
                if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):

                    addr = self.format_register(words[0])

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

                    addr = self.format_register(words[0])

                    self.transaction_list.append(wisce_script_function(words[3],
                                                                       [addr[0],  # addr
                                                                        value,  # val
                                                                        mask],  # mask
                                                                       comment))
            elif (words[0] == 'load'):
                self.load_subfile(line, raw_words, filename)

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

    def format_register(self, toFormat):
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

    def load_subfile(self, line, raw_words, filename):
        subfile = line.split(raw_words[0] + ' ')[1]
        subfile = subfile.replace('\r', '').replace('\n', '').replace('"', '').replace('"', '')
        if platform == "win32":
            subfile = subfile.replace('/', os.path.sep)
        else:
            subfile = subfile.replace('\\', os.path.sep)
        path = os.path.dirname(filename)
        subimporter = wisce_script_importer(os.path.join(path, subfile), self.command, self.symbol_list)
        self.transaction_list.extend(["Loading commands from " + subfile])
        if (len(subimporter.transaction_list) == 0) or \
           (all([(isinstance(t, str)) for t in subimporter.transaction_list])):
            self.transaction_list.extend(["[WARNING] File " + subfile + " did not contain any valid transactions"])
        else:
            self.transaction_list.extend(subimporter.transaction_list)
        self.transaction_list.extend(["Finished loading commands from " + subfile])

    def __str__(self):
        output_str = 'blah'
        return output_str

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
