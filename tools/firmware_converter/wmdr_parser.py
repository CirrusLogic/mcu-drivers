#==========================================================================
# (c) 2019 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Parser for WMFW files
# File    : wmfw_parser.py
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
import io
from wmfw_parser import wmfw_component, user_defined_name_text_block_type, metadata_block_type, \
    halo_block_types_memory_region_u24, halo_block_types_memory_region_p32, halo_block_types_memory_region_pm32, halo_block_types_memory_region_u32, \
    informational_text_block_type, absolute_addressing_data_block_type, adsp_block_types_memory_region_u24, adsp_block_types_memory_region_pm32

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
class wmdr_header(wmfw_component):

    def __init__(self, file):
        wmfw_component.__init__(self, file)
        self.fields['magic_id'] = 0
        self.fields['header_length'] = 0
        self.fields['file_format_version'] = 0
        self.fields['firmware_revision'] = 0
        self.fields['target_core'] = 0
        self.fields['core_revision'] = 0

        return

    def parse(self, file):
        wmfw_component.parse(self, file, include_header=True)

        # Verify is WMDR file
        if (self.block_header != 0x52444D57):    #'WMDR'
        #if (next_str != 'WMFW'):
            error_exit("Invalid WMDR file!")

        self.fields['magic_id'] = self.block_header

        # Header length is same as 'block_size'

        # Get file format version, firmware revisino, target core, core revision
        next_word = self.get_next_int(self.bytestream, 4)
        self.fields['firmware_revision'] = bytes_from_word(next_word, 0, 3)
        self.fields['file_format_version'] = bytes_from_word(next_word, 3, 1)
        next_word = self.get_next_int(self.bytestream, 4)
        self.fields['core_revision'] = bytes_from_word(next_word, 0, 3)
        self.fields['target_core'] = bytes_from_word(next_word, 3, 1)

        return

    def __str__(self):
        return component_to_string(self, 'WMDR Header')

class wmdr_block:

    def __init__(self, file):
        self.file_pos = file.tell()
        self.bytes = []
        self.fields = dict()
        self.fields['type'] = None
        self.fields['start_offset'] = 0
        self.fields['algorithm_identification'] = 0
        self.fields['algorithm_version'] = 0
        self.fields['sample_rate'] = 0
        self.fields['data_length'] = 0
        self.data = []
        self.memory_type = None
        return

    def parse(self, file):
        file.seek(self.file_pos)
        new_word = self.get_next_int(file, 4)

        # Get start offset
        self.fields['start_offset'] = bytes_from_word(new_word, 0, 2)

        # Get type
        self.fields['type'] = bytes_from_word(new_word, 2, 2)

        # Based on type, assign memory type
        if (self.fields['type'] in halo_block_types_memory_region_u24):
            self.memory_type = 'u24'
        elif (self.fields['type'] in halo_block_types_memory_region_pm32):
            self.memory_type = 'pm32'
        elif (self.fields['type'] in halo_block_types_memory_region_p32):
            self.memory_type = 'p32'
        elif (self.fields['type'] in halo_block_types_memory_region_u32):
            self.memory_type = 'u32'
        elif (self.fields['type'] in adsp_block_types_memory_region_u24):
            self.memory_type = 'u24'
        elif (self.fields['type'] in adsp_block_types_memory_region_pm32):
            self.memory_type = 'pm32'

        # Get algorithm identification
        self.fields['algorithm_identification'] = self.get_next_int(file, 4)
        self.fields['algorithm_version'] = self.get_next_int(file, 4)
        self.fields['sample_rate'] = self.get_next_int(file, 4)
        self.fields['data_length'] = self.get_next_int(file, 4)

        # Read data payload
        for i in range(0, self.fields['data_length']):
            self.data.append(file.read(1))

        return

    def get_next_int(self, file, num_bytes):
        new_byte_str = file.read(num_bytes)
        self.bytes.append(new_byte_str)
        new_word = bytestr_to_int(new_byte_str, num_bytes)

        return new_word

    def __str__(self):
        return component_to_string(self, 'Coefficient Value Data Block')

class wmdr_user_defined_name_block(wmdr_block):
    def __init__(self, file):
        wmdr_block.__init__(self, file)
        self.text = ''
        return

    def parse(self, file):
        wmdr_block.parse(self, file)

        # Read padding bytes
        # Padding bytes is 4 - ((length byte + name_length bytes) modulo 4)
        padding_bytes_total = get_padding_bytes(self.fields['data_length'])
        for i in range(0, padding_bytes_total):
            self.get_next_int(file, 1)

        # Convert data into text
        self.text = b''.join(self.data).decode('utf-8')

        return

    def __str__(self):
        output_str = component_to_string(self, 'User Defined Name Block')
        output_str = output_str + "--text: " + self.text + "\n"
        return output_str

class wmdr_metadata_block(wmdr_block):
    def __init__(self, file):
        wmdr_block.__init__(self, file)
        self.text = ''
        return

    def parse(self, file):
        wmdr_block.parse(self, file)

        # Read padding bytes
        # Padding bytes is 4 - ((length byte + name_length bytes) modulo 4)
        padding_bytes_total = get_padding_bytes(self.fields['data_length'])
        for i in range(0, padding_bytes_total):
            self.get_next_int(file, 1)

        # Convert data into text
        self.text = b''.join(self.data).decode('utf-8')

        return

    def __str__(self):
        output_str = component_to_string(self, 'User Defined Name Block')
        output_str = output_str + "--meta: " + self.text + "\n"
        return output_str


class wmdr_informational_text_data_block(wmdr_block):

    def __init__(self, file):
        wmdr_block.__init__(self, file)
        self.text = ''
        return

    def parse(self, file):
        wmdr_block.parse(self, file)

        # Read padding bytes
        # Padding bytes is 4 - ((length byte + name_length bytes) modulo 4)
        padding_bytes_total = get_padding_bytes(self.fields['data_length'])
        for i in range(0, padding_bytes_total):
            self.get_next_int(file, 1)

        # Convert data into text
        self.text = b''.join(self.data).decode('utf-8')

        return

    def __str__(self):
        output_str = component_to_string(self, 'Informational Text Data Block')
        output_str = output_str + "--text: " + self.text + "\n"
        return output_str

class wmdr_absolute_addressing_data_block(wmdr_block):

    def __init__(self, file):
        wmdr_block.__init__(self, file)
        self.text = ''
        return

    def parse(self, file):
        wmdr_block.parse(self, file)
        return

    def __str__(self):
        output_str = component_to_string(self, 'Absolute Addressing Data Block')
        output_str += "--text: " + self.text + "\n"
        return output_str

class wmdr_parser:

    def __init__(self, filename):
        self.filename = filename
        self.user_defined_name_block = None
        self.metadata_block = None
        self.coefficient_value_data_blocks = []
        self.informational_text_blocks = []
        self.absolute_addressing_data_blocks = []
        self.data_blocks = []

        return

    def parse(self):
        f = open(self.filename, 'rb')

        # Make header
        self.header = wmdr_header(f)
        self.header.parse(f)

        # Get blocks
        while (not f.tell() == os.fstat(f.fileno()).st_size):
            temp_block_type = file_int_peek(f, 4)
            # Here, unlike in WMFW files, there is mixing of 2-byte block types and 1-byte block types, so
            # block type is in 2-bytes
            temp_block_type = bytes_from_word(temp_block_type, 2, 2)
            if (temp_block_type == (user_defined_name_text_block_type << 8)):
                self.user_defined_name_block = wmdr_user_defined_name_block(f)
                self.user_defined_name_block.parse(f)
            elif (temp_block_type == (metadata_block_type << 8)):
                self.metadata_block = wmdr_metadata_block(f)
                self.metadata_block.parse(f)
            elif (temp_block_type == (informational_text_block_type << 8)):
                self.informational_text_blocks.append(wmdr_informational_text_data_block(f))
                self.informational_text_blocks[-1].parse(f)
            else:
                if (temp_block_type == (absolute_addressing_data_block_type << 8)):
                    new_block = wmdr_absolute_addressing_data_block(f)
                else:
                    new_block = wmdr_block(f)

                self.data_blocks.append(new_block)
                self.data_blocks[-1].parse(f)

        return

    def __str__(self):
        output_str = str(self.header) + "\n"
        output_str = output_str + str(self.user_defined_name_block) + "\n"
        if (self.metadata_block):
            output_str = output_str + str(self.metadata_block) + "\n"
        for block in self.data_blocks:
            output_str = output_str + str(block) + "\n"
        for block in self.absolute_addressing_data_blocks:
            output_str = output_str + str(block) + "\n"
        for block in self.informational_text_blocks:
            output_str = output_str + str(block) + "\n"

        return output_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================
def validate_environment():
    result = True

    return result

def validate_args(args):
    result = False

    if (len(args) == 2):
        # Check that WMDR path exists
        if (os.path.exists(args[1])):
            result = True

    return result

def print_start():
    print("")
    print("wmdr_parser")
    print("Parser for WMDR files")
    print("Version " + VERSION_STRING)

    return

def print_usage():
    print("")
    print("Usage:")
    print("    python wmdr_parser.py <wmdr_path>")

    return

def print_args(args):
    print("")
    print("WMDR Path: " + args[1])

    return

def print_results(results_string):
    print(results_string)

    return

def print_end():
    print("Exit.")

    return

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

def bytes_from_word(word, byte_pos, num_bytes):
    bit_shift = byte_pos * 8
    bit_mask = ~(0xFFFFFFFF << (num_bytes * 8))
    bit_mask = (bit_mask << bit_shift)
    return ((word & bit_mask) >> bit_shift)

def file_byte_peek(file):
    file_pos = file.tell()
    byte_str = file.read(1)
    file.seek(file_pos)

    return byte_str

def file_int_peek(file, word_size_bytes):
    file_pos = file.tell()
    byte_str = file.read(word_size_bytes)
    file.seek(file_pos)

    return bytestr_to_int(byte_str, word_size_bytes)

def get_padding_bytes(length):
    rem = length % 4
    if (rem != 0):
        return (4 - rem)
    else:
        return 0

def memory_type_converter(from_type, to_type, word_list):
    new_list = []

    new_byte_list = []
    if ((from_type == 'p32') and ((len(word_list) % 3) == 0)):

        for i in range(0, len(word_list)):
            new_byte_list.append(word_list[i] & 0xFF)
            new_byte_list.append((word_list[i] & 0xFF00) >> 8)
            new_byte_list.append((word_list[i] & 0xFF0000) >> 16)
            new_byte_list.append((word_list[i] & 0xFF000000) >> 24)

    # Remember need to switch endianness here also
    if ((to_type == 'u24') and (len(new_byte_list) > 0)):
        for i in range(0, int(len(new_byte_list) / 3)):
            new_word = new_byte_list[i * 3]
            new_word = new_word + (new_byte_list[i * 3 + 1] << 8)
            new_word = new_word + (new_byte_list[i * 3 + 2] << 16)
            new_list.append(new_word)

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

#==========================================================================
# MAIN PROGRAM
#==========================================================================
def main(argv):

    print_start()

    if (not (validate_environment())):
        error_exit("Invalid Environment")

    # validate arguments
    if (not (validate_args(argv))):
        print_usage()
        error_exit("Invalid Arguments")
    else:
        print_args(argv)

    p = wmdr_parser(argv[1])
    p.parse()

    print_results(str(p))

    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
