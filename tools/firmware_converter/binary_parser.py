#==========================================================================
# (c) 2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Parser for arbitrary bin files
# File    : binary_parser.py
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

#==========================================================================
# VERSION
#==========================================================================
VERSION_STRING = "1.0.0"

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class bin_block:

    def __init__(self, file, address, mem_type, mem_start):
        self.file_pos = file.tell()
        self.bytes = []
        self.fields = dict()
        self.fields['type'] = 'abs'
        self.fields['address'] = 0
        self.fields['start_address'] = address
        self.fields['data_length'] = 0
        self.fields['mem_type'] = mem_type
        self.fields['mem_start'] = mem_start
        self.data = []
        self.memory_type = None
        return

    def parse(self, file):
        file.seek(self.file_pos)

        # Get start offset
        self.fields['address'] = self.fields['start_address'] + self.file_pos

        # Packed addresses must be on 12-byte boundaries
        if (self.fields['mem_type'] == 'p32'):
            if ((self.fields['address'] - self.fields['mem_start']) % 12):
                self.fields['address'] = self.fields['address'] - ((self.fields['address'] - self.fields['mem_start']) % 12)
                print("WARNING Binary parser: packed address not on 12-byte boundary. Address changed to " + hex(self.fields['address']) + " to align properly.")
        # All addresses must be on 4-byte boundaries
        elif self.fields['address'] & 0x3:
            self.fields['address'] = self.fields['address'] & ~0x3
            print("WARNING Binary parser: address not on 4-byte boundary. Address changed to " + hex(self.fields['address']) + " to align properly.")

        # Read all data, redistribute in rehash_blocks
        while (not file.tell() == os.fstat(file.fileno()).st_size):
            self.data.append(file.read(1))

        # data alignment and padding
        self.data = memory_type_converter('binary', self.fields['mem_type'], self.data)

        self.fields['data_length'] = len(self.data)
        return

    def get_next_int(self, file, num_bytes):
        new_byte_str = file.read(num_bytes)
        self.bytes.append(new_byte_str)
        new_word = bytestr_to_int(new_byte_str, num_bytes)

        return new_word

    def __str__(self):
        return component_to_string(self, 'Bin Data Block')

class bin_parser:

    def __init__(self, bin_dict, mem_tuple):
        self.filename = bin_dict['path']
        self.address = bin_dict['addr']
        self.mem_type = mem_tuple[0]
        self.mem_start = mem_tuple[1]
        self.absolute_addressing_data_blocks = []
        self.data_blocks = []

        return

    def parse(self):
        f = open(self.filename, 'rb')

        # Get block
        new_block = bin_block(f, self.address, self.mem_type, self.mem_start)

        self.data_blocks.append(new_block)
        self.data_blocks[-1].parse(f)

        return

    def __str__(self):
        output_str = ''
        for block in self.data_blocks:
            output_str = output_str + str(block) + "\n"
        for block in self.absolute_addressing_data_blocks:
            output_str = output_str + str(block) + "\n"

        return output_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def error_exit(error_message):
    print('ERROR: ' + error_message)
    exit(1)

def bytestr_to_int(bytestr, word_size):
    temp_int = 0
    if (len(bytestr) < word_size):
        error_exit("bytestr_to_int failure!")
    else:
        for i in range(0, word_size):
            temp_int = temp_int + (list(bytestr)[i] << (i * 8))

    return temp_int

def memory_type_converter(from_type, to_type, byte_list):

    if (from_type == 'binary') and (len(byte_list) > 0):
        if (to_type == 'u24'):
            new_list = []
            if (len(byte_list) % 3) != 0:
                byte_list.append(bytes.fromhex('00') * (len(byte_list) % 3))

            for i in range(0, int(len(byte_list) / 3)):
                new_list.append(bytes.fromhex('00'))
                new_list.append(byte_list[i * 3])
                new_list.append(byte_list[i * 3 + 1])
                new_list.append(byte_list[i * 3 + 2])

        elif (to_type == 'p32'):
            new_list = []
            if (len(byte_list) % 12) != 0:
                byte_list.append(bytes.fromhex('00') * (len(byte_list) % 12))

            for i in range(0, int(len(byte_list) / 12)):
                # all 24-bits of word 1 + 8 bits of word 2
                new_list.append(byte_list[i * 12 + 5])
                new_list.append(byte_list[i * 12])
                new_list.append(byte_list[i * 12 + 1])
                new_list.append(byte_list[i * 12 + 2])
                # the remaining 16-bits of word 2 + the first 16-bits of word 3
                new_list.append(byte_list[i * 12 + 7])
                new_list.append(byte_list[i * 12 + 8])
                new_list.append(byte_list[i * 12 + 3])
                new_list.append(byte_list[i * 12 + 4])
                # the remaining 8-bits of word 3 + all 24-bits of word 4
                new_list.append(byte_list[i * 12 + 9])
                new_list.append(byte_list[i * 12 + 10])
                new_list.append(byte_list[i * 12 + 11])
                new_list.append(byte_list[i * 12 + 6])

        else:
            new_list = []
            error_exit("Binary parser: Unsupported target memory range")
    else:
        new_list = []
        error_exit("Binary parser: Original data empty or in unsupported type")

    return new_list

def component_to_string(component, name):
    output_str = name + ':\n'
    for key in component.fields:
        if (type(component.fields[key]) == dict):
            output_str = output_str + "--" + key + ":\n"
            for sub_key in component.fields[key]:
                if (type(component.fields[key][sub_key]) == int):
                    output_str = output_str + "----" + sub_key + ": " + hex(component.fields[key][sub_key]) + "\n"
                else:
                    output_str = output_str + "----" + sub_key + ": " + str(component.fields[key][sub_key]) + "\n"
        elif (type(component.fields[key]) == list):
            output_str = output_str + "--" + key + ":\n"
            for element in component.fields[key]:
                if (type(element) == int):
                    output_str = output_str + "----" + hex(element) + "\n"
                else:
                    output_str = output_str + "----" + str(element) + "\n"
        elif (type(component.fields[key]) == int):
            output_str = output_str + "--" + key + ": " + hex(component.fields[key]) + "\n"
        elif (type(component.fields[key]) == str):
            output_str = output_str + "--" + key + ": " + component.fields[key] + "\n"

    return output_str

