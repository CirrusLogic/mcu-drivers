#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Import WISCE Script transactions
# File    : wisce_script_importer.py
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
from wisce_script_transaction import wisce_script_transaction

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
wisce_script_keywords_to_skip = ['load',
                                 'wait',
                                 'if',
                                 'else',
                                 'endif',
                                 'prompt_to_continue',
                                 'exit',
                                 'insert_delay',
                                 'loadfirmware',
                                 'block_write',
                                 'SMbus_16inx_16dat']
wisce_script_comment = ['*', 'message']
wisce_script_transaction_keywords = ['4wireSPI_32inx_32dat', '4wireSPI_32inx_16dat', 'SMbus_32inx_32dat']

#==========================================================================
# CLASSES
#==========================================================================
class wisce_script_importer:
    def __init__(self, filename):
        self.transaction_list = []

        f = open(filename)
        for line in f.readlines():
            # Check for starting with a keyword
            words = line.split(' ')
            words = [w for w in words if w != '']
            if words[0] in wisce_script_keywords_to_skip:
                continue
            if words[0] in wisce_script_comment:
                continue

            if (any(access_type in word for access_type in wisce_script_transaction_keywords for word in words)):
                # First check that it is a 'Write' transaction
                if (words[3] == 'Write'):
                    # Get any comments
                    comment = None
                    if (len(line.split('*')) > 1):
                        comment = line.split('*')[1]

                    self.transaction_list.append(wisce_script_transaction(int(words[0], 16),
                                                                          int(words[1], 16),
                                                                          comment))

        f.close()
        return

    def get_transaction_list(self):
        return self.transaction_list

    def __str__(self):
        output_str = 'blah'
        return output_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

