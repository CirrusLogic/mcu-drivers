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
from wisce_script_transaction import wisce_script_transaction
from wisce_script_function import wisce_script_function
from re import search

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
wisce_script_keywords_to_skip_array = ['load',
                                       'loadbinary',
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
wisce_script_keywords_to_skip_functions = ['load',
                                           'loadbinary',
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
    def __init__(self, filename, command):
        self.transaction_list = []
        self.command = command
        if (self.command == 'c_array'):
            wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_array
        elif (self.command == 'c_functions'):
            wisce_script_keywords_to_skip = wisce_script_keywords_to_skip_functions

        f = open(filename)
        iter_lines = iter(f.readlines())
        for line in iter_lines:
            # Check for starting with a keyword
            line = line.replace('\t', ' ')
            words = line.split(' ')
            words = [w for w in words if w != '']
            words = [word.lower() for word in words]
            if words[0] in wisce_script_keywords_to_skip:
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
                        self.transaction_list.append(wisce_script_transaction(int(words[0], 16),
                                                                              int(words[1], 16),
                                                                              comment))
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

                    self.transaction_list.append(wisce_script_function(words[0],
                                                                       [int(words[1], 16),
                                                                        data],
                                                                       comment))
                elif (len(words) > 3) and (words[3] == 'write'):
                    if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
                        self.transaction_list.append(wisce_script_function(words[3],
                                                                           [int(words[0], 16),
                                                                            int(words[1], 16)],
                                                                           comment))
                elif (len(words) > 3) and (words[3] == 'rmodw'):
                    if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in
                            words)):
                        if len(words) >= 6 and (words[5].find("*") == -1) and words[5] != 'message':
                            mask = int(words[5], 16)
                        else:
                            mask = 0xffffffff
                        self.transaction_list.append(wisce_script_function(words[3],
                                                                           [int(words[0], 16),  # addr
                                                                            int(words[1], 16),  # val
                                                                            mask],  # mask
                                                                           comment))

        f.close()
        return

    def get_transaction_list(self):
        return self.transaction_list

    def __str__(self):
        output_str = 'blah'
        return output_str

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
