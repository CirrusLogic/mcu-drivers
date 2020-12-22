#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Class definition and parser for fw_img_v1
# File    : fw_img_v1.py
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

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class fw_img_v1:
    def __init__(self):
        # Header
        self.img_magic_number_1 = 0x54b998ff
        self.img_format_rev = 1
        self.img_size = (10 * 4) # Header + Footer words * 4 words/byte
        self.sym_table_size = 0
        self.alg_list_size = 0
        self.fw_id = 0
        self.fw_version = 0
        self.data_blocks = 0

        # Symbol Linking Table
        self.symbol_linking_table = []

        # Algorithm ID List
        self.algorithm_id_list = []

        # Payload Data
        self.payload_data_blocks = []

        # Footer
        self.img_magic_number_2 = 0x936be2a6
        self.img_checksum = 0

        return

    def add_symbol_table_entry(self, id, address):
        self.symbol_linking_table.append((id, address))
        self.sym_table_size = self.sym_table_size + 1
        self.img_size = self.img_size + (2 * 4)

        return

    def add_algorithm_id(self, id):
        self.alg_list_size = self.alg_list_size + 1
        self.algorithm_id_list.append(id)
        self.img_size = self.img_size + 4

        return

    def add_payload_data_block(self, size_bytes, address, payload_bytes):
        self.data_blocks = self.data_blocks + 1
        self.img_size = self.img_size + size_bytes
        self.payload_data_blocks.append((size_bytes, address, payload_bytes))

        return

    def integrity_check(self):
        # check magic numbers
        if ((self.img_magic_number_1 != 0x54b998ff) or (self.img_magic_number_2 != 0x936be2a6)):
            print("Incorrect magic number(s)")
            return False

        # check list sizes are correct
        if (self.sym_table_size != len(self.symbol_linking_table)):
            print("Incorrect sym_table_size")
            return False

        if (self.alg_list_size != len(self.algorithm_id_list)):
            print("Incorrect alg_list_size")
            return False

        if (self.data_blocks != len(self.payload_data_blocks)):
            print("Incorrect data_blocks")
            return False

        size = (10 + (self.sym_table_size * 2) + self.alg_list_size) * 4
        for d in self.payload_data_blocks:
            size = size + 8 + d[0]

        if (self.img_size != size):
            print(self.img_size)
            print(size)
            print("Incorrect img_size")
            return False

        # check checksum

        return True

    def update_checksum(self):
        return

    def __str__(self):
        lines = []
        lines.append("fw_img_v1 contents:")

        lines.append("Header:")
        lines.append("    IMG_MAGIC_NUMBER_1 : " + hex(self.img_magic_number_1))
        lines.append("    IMG_FORMAT_REV     : " + hex(self.img_format_rev))
        lines.append("    IMG_SIZE           : " + hex(self.img_size))
        lines.append("    SYM_TABLE_SIZE     : " + hex(self.sym_table_size))
        lines.append("    ALG_LIST_SIZE      : " + hex(self.alg_list_size))
        lines.append("    FW_ID              : " + hex(self.fw_id))
        lines.append("    FW_VERSION         : " + hex(self.fw_version))
        lines.append("    DATA_BLOCKS        : " + hex(self.data_blocks))

        lines.append("Symbol Linking Table:")
        for i in range(0, len(self.symbol_linking_table)):
            lines.append("    SYM_ID_" + str(i) + '\t\t: ' + hex(self.symbol_linking_table[i][0]))
            lines.append("    SYM_ADDR_" + str(i) + '\t\t: ' + hex(self.symbol_linking_table[i][1]))

        lines.append("Algorithm ID List:")
        for i in range(0, len(self.algorithm_id_list)):
            lines.append("    ALG_ID_" + str(i) + '\t\t: ' + hex(self.algorithm_id_list[i]))

        lines.append("Payload Data:")
        for i in range(0, len(self.payload_data_blocks)):
            lines.append("    BLOCK_SIZE_" + str(i) + '\t\t: ' + hex(self.payload_data_blocks[i][0]))
            lines.append("    BLOCK_ADDR_" + str(i) + '\t\t: ' + hex(self.payload_data_blocks[i][1]))
            lines.append("    BLOCK_PAYLOAD_" + str(i) + '\t\t: ')
            byte_list = []
            for b in self.payload_data_blocks[i][2]:
                byte_list.append(hex(b))
                if (len(byte_list) >= 32):
                    lines.append(', '.join(byte_list))
                    byte_list = []
            if (len(byte_list)):
                lines.append(', '.join(byte_list))

        lines.append("Footer:")
        lines.append("    IMG_MAGIC_NUMBER_2 : " + hex(self.img_magic_number_2))
        lines.append("    IMG_CHECKSUM       : " + hex(self.img_checksum))
        lines.append("")

        return '\n'.join(lines)

class fw_img_v1_parser:
    def __init__(self):
        self.img = None

        return

    def parse_bytes(self, bytes):
        # Check that number of bytes is word aligned
        if (len(bytes) % 4):
            return False

        # Get words from bytes
        words = []
        for i in range(0, len(bytes), 4):
            word = bytes[i]
            word = word | (bytes[i + 1] << 8)
            word = word | (bytes[i + 2] << 16)
            word = word | (bytes[i + 3] << 24)
            words.append(word)

        self.img = fw_img_v1()

        # Load Header words
        self.img.img_magic_number_1 = words[0]
        self.img.img_format_rev = words[1]
        self.img.fw_id = words[5]
        self.img.fw_version = words[6]
        if self.img.img_format_rev == 1:
            word_count = 8
        else:
            word_count = 10

        # Load Symbol Linking Table words
        for i in range(0, words[3]):
            self.img.add_symbol_table_entry(words[word_count + (i * 2)], words[word_count + (i * 2) + 1])
        if (words[3] != self.img.sym_table_size):
            return False
        word_count = word_count + (self.img.sym_table_size * 2)

        # Load Algorithm ID List
        for i in range(0, words[4]):
            self.img.add_algorithm_id(words[word_count + i])
        if (words[4] != self.img.alg_list_size):
            return False
        word_count = word_count + self.img.alg_list_size

        # Load Payload Data blocks
        for i in range(0, words[7]):
            block_size = words[word_count]
            if (block_size % 4):
                return False

            word_count = word_count + 1

            block_address = words[word_count]

            word_count = word_count + 1

            block_payload_bytes = []
            for j in range(0, (block_size >> 2)):
                block_payload_bytes.append(words[word_count] & 0x000000FF)
                block_payload_bytes.append((words[word_count] >> 8) & 0x000000FF)
                block_payload_bytes.append((words[word_count] >> 16) & 0x000000FF)
                block_payload_bytes.append((words[word_count] >> 24) & 0x000000FF)
                word_count = word_count + 1

            self.img.add_payload_data_block(block_size, block_address, block_payload_bytes)

        if (words[7] != self.img.data_blocks):
            return False

        # Load Footer
        self.img.img_magic_number_2 = words[word_count]
        word_count = word_count + 1
        self.img.img_checksum = words[word_count]
        word_count = word_count + 1

        # Check img_size
        self.img.img_size = word_count * 4
        if (words[2] != self.img.img_size):
            return False

        return self.img.integrity_check()

    def parse_header(self, filename):
        if (os.path.exists(filename)):
            self.filename = filename
            bytes = self.get_fw_img_file_bytes(filename)

            return self.parse_bytes(bytes)

        return False

    def parse_binary(self, filename):
        if (os.path.exists(filename)):
            self.filename = filename
            bytes = []
            f = open(filename, 'rb')
            byte = f.read(1)
            while byte:
                bytes.append(int.from_bytes(byte, "little"))
                byte = f.read(1)

            return self.parse_bytes(bytes)

        return False

    def get_fw_img_file_bytes(self, filename):
        bytes = None

        # Check file exists
        if (os.path.exists(filename)):
            f = open(filename, 'r')
            found_array_start = False
            bytes = []
            for line in f.readlines():
                line_bytes = []

                if ((not found_array_start) and ('[] = {' in line)):
                    found_array_start = True
                    continue

                if (found_array_start):
                    found_0 = False
                    found_x = False
                    for c in line:
                        if (c == '/'):
                            break
                        if ((not found_0) and (c == '0')):
                            found_0 = True
                            continue
                        if ((found_0) and (not found_x)):
                            if (c == 'x'):
                                found_x = True
                                got_first_c = False
                                temp_hex = '0x'
                            else:
                                found_0 = False
                                found_x = False
                            continue

                        if ((found_0) and (found_x)):
                            if (not got_first_c):
                                temp_hex = temp_hex + c
                                got_first_c = True
                            else:
                                temp_hex = temp_hex + c
                                line_bytes.append(int(temp_hex, 16))
                                found_0 = False
                                found_x = False

                bytes.extend(line_bytes)

            f.close()

        return bytes

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

