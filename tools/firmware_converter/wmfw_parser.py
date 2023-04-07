#==========================================================================
# (c) 2019, 2021-2023 Cirrus Logic, Inc.
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

#==========================================================================
# VERSION
#==========================================================================
VERSION_STRING = "1.0.0"

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
adsp_pm_p32_block_type = 0x02
adsp_zm_u24_block_type = 0x04
adsp_xm_u24_block_type = 0x05
adsp_ym_u24_block_type = 0x06
adsp_block_types_memory_region_u24 = [
    adsp_xm_u24_block_type,
    adsp_ym_u24_block_type,
    adsp_zm_u24_block_type]
adsp_block_types_memory_region_pm32 = [
    adsp_pm_p32_block_type]
adsp_memory_region_block_types = [
    adsp_xm_u24_block_type,
    adsp_ym_u24_block_type,
    adsp_zm_u24_block_type,
    adsp_pm_p32_block_type]

halo_xm_u24_block_type = 0x05
halo_ym_u24_block_type = 0x06
halo_pm_p32_block_type = 0x10
halo_xm_p32_block_type = 0x11
halo_ym_p32_block_type = 0x12
halo_xm_u32_block_type = 0x21
halo_ym_u32_block_type = 0x22
halo_block_types_memory_region_u24 = [
    halo_xm_u24_block_type,
    halo_ym_u24_block_type]
halo_block_types_memory_region_pm32 = [
    halo_pm_p32_block_type]
halo_block_types_memory_region_p32 = [
    halo_xm_p32_block_type,
    halo_ym_p32_block_type]
halo_block_types_memory_region_u32 = [
    halo_xm_u32_block_type,
    halo_ym_u32_block_type]
halo_memory_region_block_types = [
    halo_xm_u24_block_type,
    halo_ym_u24_block_type,
    halo_pm_p32_block_type,
    halo_xm_p32_block_type,
    halo_ym_p32_block_type,
    halo_xm_u32_block_type,
    halo_ym_u32_block_type]

halo_block_types_memory_region_xm = [
    halo_xm_u24_block_type,
    halo_xm_p32_block_type,
    halo_xm_u32_block_type]
halo_block_types_memory_region_ym = [
    halo_ym_u24_block_type,
    halo_ym_p32_block_type,
    halo_ym_u32_block_type]
halo_block_types_memory_region_pm = [
    halo_pm_p32_block_type]

informational_text_block_type = 0xFF
algorithm_information_data_block = 0xF2
user_defined_name_text_block_type = 0xFE
metadata_block_type = 0xFC
absolute_addressing_data_block_type = 0xF0

adsp2_v1_max_name_length = 256 # bytes

#==========================================================================
# CLASSES
#==========================================================================
class wmfw_component:

    def __init__(self, file):
        self.file_pos = file.tell()
        self.bytes = []
        self.fields = dict()
        self.fields['block_size'] = 0
        self.checksum = 0
        self.block_header = 0
        self.bytestream = io.BytesIO()

        return

    def parse(self, file, include_header=False):
        file.seek(self.file_pos)
        self.block_header = self.get_next_int(file, 4)
        self.fields['block_size'] = self.get_next_int(file, 4)
        if (include_header):
            self.bytestream = io.BytesIO(file.read(self.fields['block_size'] - 8))
        else:
            self.bytestream = io.BytesIO(file.read(self.fields['block_size']))

        return

    def get_next_str(self, file):
        new_byte_str = file.read(4)
        self.bytes.append(new_byte_str)
        new_str = new_byte_str.decode('utf-8')

        return new_str


    def get_next_int(self, file, num_bytes):
        new_byte_str = file.read(num_bytes)
        self.bytes.append(new_byte_str)
        new_word = bytestr_to_int(new_byte_str, num_bytes)

        return new_word

    def sum_bytes(self):
        for byte_str in self.bytes:
            self.checksum = self.checksum + bytestr_to_int(byte_str, 4)

        return self.checksum

class wmfw_header(wmfw_component):

    def __init__(self, file):
        wmfw_component.__init__(self, file)
        self.fields['magic_id'] = 0
        self.fields['header_length'] = 0
        self.fields['file_format_version'] = 0
        self.fields['target_core'] = 0
        self.fields['api_revision'] = 0
        self.fields['memory_sizes'] = dict()
        self.fields['creation_timestamp'] = 0
        self.fields['checksum'] = 0

        return

    def parse(self, file):
        wmfw_component.parse(self, file, include_header=True)

        # Verify is WMFW file
        if (self.block_header != 0x57464D57):    #'WMFW'
        #if (next_str != 'WMFW'):
            error_exit("Invalid WMFW file!")

        self.fields['magic_id'] = self.block_header

        # Header length is same as 'block_size'

        # Get file format version, target core, api revision
        next_word = self.get_next_int(self.bytestream, 4)
        self.fields['file_format_version'] = bytes_from_word(next_word, 3, 1)
        self.fields['target_core'] = bytes_from_word(next_word, 2, 1)
        self.fields['api_revision'] = bytes_from_word(next_word, 0, 2)

        # Get memory sizes block
        if (self.fields['target_core'] != 4 and self.fields['target_core'] != 2):
            error_exit('Current core type of ' + str(self.fields['target_core']) + ' is NOT supported!')
        else:
            self.fields['memory_sizes']['xm_size'] = self.get_next_int(self.bytestream, 4)
            self.fields['memory_sizes']['ym_size'] = self.get_next_int(self.bytestream, 4)
            self.fields['memory_sizes']['pm_size'] = self.get_next_int(self.bytestream, 4)
            self.fields['memory_sizes']['zm_size'] = self.get_next_int(self.bytestream, 4)

        # Get creation timestamp
        self.fields['creation_timestamp'] = self.get_next_int(self.bytestream, 4) + (self.get_next_int(self.bytestream, 4) << 32)

        # Get checksum
        self.fields['checksum'] = self.get_next_int(self.bytestream, 4)

        return

    def __str__(self):
        return component_to_string(self, 'WMFW Header')

class wmfw_block(wmfw_component):

    def __init__(self, file):
        wmfw_component.__init__(self, file)

        self.fields['type'] = None
        return

    def parse(self, file, type_size_bytes=1):
        wmfw_component.parse(self, file)

        if (type_size_bytes == 1):
            self.fields['type'] = bytes_from_word(self.block_header, 3, 1)
        elif (type_size_bytes == 2):
            self.fields['type'] = bytes_from_word(self.block_header, 2, 2)

        return

    def __str__(self):
        return component_to_string(self, 'Block')

class wmfw_memory_region_data_block(wmfw_block):

    def __init__(self, file):
        wmfw_block.__init__(self, file)
        self.fields['start_offset'] = 0
        self.data = []
        self.memory_type = None
        return

    def parse(self, file):
        wmfw_block.parse(self, file)

        self.fields['start_offset'] = bytes_from_word(self.block_header, 0, 3)

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

        for i in range(0, self.fields['block_size']):
            self.data.append(self.bytestream.read(1))

        return

    def __str__(self):
        return component_to_string(self, 'Memory Region Data Block')

class wmfw_informational_text_data_block(wmfw_block):

    def __init__(self, file):
        wmfw_block.__init__(self, file)
        self.text = ''
        return

    def parse(self, file):
        wmfw_block.parse(self, file)

        temp_bytes = self.bytestream.read()
        self.text = temp_bytes.decode('utf-8')

        return

    def __str__(self):
        output_str = component_to_string(self, 'Informational Text Data Block')
        output_str = output_str + "--text: " + self.text + "\n"
        return output_str

class wmfw_algorithm_information_data_block(wmfw_block):

    def __init__(self, file, file_format_version):
        wmfw_block.__init__(self, file)
        self.fields['algorithm_id'] = 0
        self.fields['name_length'] = 0
        self.fields['algorithm_name'] = ''
        self.fields['description_length'] = 0
        self.fields['algorithm_description'] = ''
        self.fields['coefficient_count'] = 0
        self.fields['coefficient_descriptors'] = []
        self.file_format_version = file_format_version

        return

    def parse(self, file):
        wmfw_block.parse(self, file)

        # Get Algorithm ID
        self.fields['algorithm_id'] = self.get_next_int(self.bytestream, 4)

        # Get name length
        if self.file_format_version == 1:
            self.fields['name_length'] = adsp2_v1_max_name_length
        else:
            self.fields['name_length'] = self.get_next_int(self.bytestream, 1)

        # Get Algorithm Name
        for name_cnt in range(0, self.fields['name_length']):
            ch = chr(self.get_next_int(self.bytestream, 1))
            if self.file_format_version == 1 and ch == chr(0x0):
                break
            self.fields['algorithm_name'] = self.fields['algorithm_name'] + ch

        # For v1 update name_length field and advance the file position
        if self.file_format_version == 1:
            self.fields['name_length'] = name_cnt
            for i in range(0, adsp2_v1_max_name_length-name_cnt-1):  # Just do self.get_next_int(self.bytestream, adsp2_v1_max_name_length-i) ?
                self.get_next_int(self.bytestream, 1)

        if self.file_format_version != 1:
            # Padding bytes is 4 - ((length byte + name_length bytes) modulo 4)
            padding_bytes_total = get_padding_bytes(self.fields['name_length'] + 1)
            for i in range(0, padding_bytes_total):
                self.get_next_int(self.bytestream, 1)

        # Get description length
        if self.file_format_version == 1:
            self.fields['description_length'] = adsp2_v1_max_name_length
        else:
            self.fields['description_length'] = self.get_next_int(self.bytestream, 2)
            if (self.fields['description_length'] == 0):
                self.get_next_int(self.bytestream, 2)

        if self.fields['description_length']:
            # Get Algorithm Description
            for descpn_cnt in range(0, self.fields['description_length']):
                ch = chr(self.get_next_int(self.bytestream, 1))
                if self.file_format_version == 1 and ch == chr(0x0):
                    break
                self.fields['algorithm_description'] = self.fields['algorithm_description'] + ch

            # For v1 update name_length field and advance the file position
            if self.file_format_version == 1:
                self.fields['description_length'] = descpn_cnt
                for i in range(0, adsp2_v1_max_name_length-descpn_cnt-1):  # Just do self.get_next_int(self.bytestream, adsp2_v1_max_name_length-i) ?
                    self.get_next_int(self.bytestream, 1)

            if self.file_format_version != 1:
                # Padding bytes is 4 - ((description length bytes + name_length bytes) modulo 4)
                padding_bytes_total = get_padding_bytes(self.fields['description_length'] + 2)
                for i in range(0, padding_bytes_total):
                    self.get_next_int(self.bytestream, 1)

        # Get coefficient count
        self.fields['coefficient_count'] = self.get_next_int(self.bytestream, 4)

        # Get coefficient blocks
        for i in range(0, self.fields['coefficient_count']):
            self.fields['coefficient_descriptors'].append(wmfw_coefficient_descriptor_data_block(self.bytestream, self.file_format_version))
            self.fields['coefficient_descriptors'][-1].parse(self.bytestream)

        return

    def __str__(self):
        return component_to_string(self, 'Algorithm Information Data Block')

class wmfw_coefficient_descriptor_data_block(wmfw_block):
    def __init__(self, file, file_format_version):
        wmfw_block.__init__(self, file)

        self.fields['start_offset'] = 0
        self.fields['name_length'] = 0
        self.fields['coefficient_name'] = ''
        self.fields['full_name_length'] = 0
        self.fields['full_coefficient_name'] = ''
        self.fields['description_length'] = 0
        self.fields['coefficient_description'] = ''
        self.fields['coefficient_type'] = 0
        self.fields['coefficient_flags'] = 0
        self.fields['control_length'] = 0
        self.fields['coefficient_info_block'] = []
        self.file_format_version = file_format_version

        return

    def parse(self, file):
        wmfw_block.parse(self, file, type_size_bytes=2)

        # Get start offset
        self.fields['start_offset'] = bytes_from_word(self.block_header, 0, 2)

        # Get name length
        if self.file_format_version == 1:
            self.fields['name_length'] = adsp2_v1_max_name_length
        else:
            self.fields['name_length'] = self.get_next_int(self.bytestream, 1)

        # Get coefficient name
        for coeff_cnt in range(0, self.fields['name_length']):
            ch = chr(self.get_next_int(self.bytestream, 1))
            if self.file_format_version == 1 and ch == chr(0x0):
                break
            self.fields['coefficient_name'] = self.fields['coefficient_name'] + ch

        # For v1 update name_length field and advance the file position
        if self.file_format_version == 1:
            self.fields['name_length'] = coeff_cnt
            for i in range(0, adsp2_v1_max_name_length-coeff_cnt-1):  # Just do self.get_next_int(self.bytestream, adsp2_v1_max_name_length-i) ?
                self.get_next_int(self.bytestream, 1)

        if self.file_format_version != 1:
            # Padding bytes is 4 - ((length byte + name_length bytes) modulo 4)
            padding_bytes_total = get_padding_bytes(self.fields['name_length'] + 1)
            for i in range(0, padding_bytes_total):
                self.get_next_int(self.bytestream, 1)

        if self.file_format_version != 1:   # v1 has no full name field
            # Get full name length
            self.fields['full_name_length'] = self.get_next_int(self.bytestream, 1)

            if (self.fields['full_name_length'] == 0):
                # Skip next 3 bytes
                self.get_next_int(self.bytestream, 3)
            else:
                # Get full name
                for i in range(0, self.fields['full_name_length']):
                    self.fields['full_coefficient_name'] = self.fields['full_coefficient_name'] + chr(self.get_next_int(self.bytestream, 1))
                # Padding bytes is 4 - ((length bytes + name_length bytes) modulo 4)
                padding_bytes_total = get_padding_bytes(self.fields['full_name_length'] + 1)
                for i in range(0, padding_bytes_total):
                    self.get_next_int(self.bytestream, 1)

        # Get coeff description length
        if self.file_format_version == 1:
            self.fields['description_length'] = adsp2_v1_max_name_length
        else:
            self.fields['description_length'] = self.get_next_int(self.bytestream, 2)
            if (self.fields['description_length'] == 0):
                # Skip next 2 bytes
                self.get_next_int(self.bytestream, 2)

        if self.fields['description_length']:
            # Get coeff description
            for descpn_cnt in range(0, self.fields['description_length']):
                ch = chr(self.get_next_int(self.bytestream, 1))
                if self.file_format_version == 1 and ch == chr(0x0):
                    break
                self.fields['coefficient_description'] = self.fields['coefficient_description'] + ch

            # For v1 update name_length field and advance the file position
            if self.file_format_version == 1:
                self.fields['description_length'] = descpn_cnt
                for i in range(0, adsp2_v1_max_name_length-descpn_cnt-1):  # Just do self.get_next_int(self.bytestream, adsp2_v1_max_name_length-i) ?
                    self.get_next_int(self.bytestream, 1)

            if self.file_format_version != 1:
                # Padding bytes is 4 - ((length bytes + name_length bytes) modulo 4)
                padding_bytes_total = get_padding_bytes(self.fields['description_length'] + 2)
                for i in range(0, padding_bytes_total):
                    self.get_next_int(self.bytestream, 1)

        # Get coefficient type and flags
        self.fields['coefficient_type'] = self.get_next_int(self.bytestream, 2)
        self.fields['coefficient_flags'] = self.get_next_int(self.bytestream, 2)

        # Get control length
        self.fields['control_length'] = self.get_next_int(self.bytestream, 4)

    def __str__(self):
        return component_to_string(self, 'Coefficient Descriptor Data Block')


def unpack_memory(bytestream, convert=False, from_mem_type='', to_mem_type=''):
    new_word_list = []
    new_bytes_list = list(bytestream.read())
    temp = int(len(new_bytes_list) / 4)
    for i in range(0, int(len(new_bytes_list) / 4)):
        new_word = new_bytes_list[i * 4]
        new_word = new_word << 8
        new_word = new_word + new_bytes_list[i * 4 + 1]
        new_word = new_word << 8
        new_word = new_word + new_bytes_list[i * 4 + 2]
        new_word = new_word << 8
        new_word = new_word + new_bytes_list[i * 4 + 3]
        new_word_list.append(new_word)

    if convert:
        return memory_type_converter(from_mem_type, to_mem_type, new_word_list)
    else:
        return new_word_list


class halo_firmware_id_block:

    def __init__(self, memory_type):
        self.memory_type = memory_type
        self.fields = dict()
        self.fields['core_id'] = 0
        self.fields['format_version'] = 0
        self.fields['vendor_id'] = 0
        self.fields['firmware_id'] = 0
        self.fields['firmware_revision'] = 0
        self.fields['sys_config_mem_offsets'] = dict()
        self.fields['sys_config_mem_offsets']['xm_base'] = 0
        self.fields['sys_config_mem_offsets']['xm_size'] = 0
        self.fields['sys_config_mem_offsets']['ym_base'] = 0
        self.fields['sys_config_mem_offsets']['ym_size'] = 0
        self.fields['number_of_algorithms'] = 0
        self.fields['algorithm_info'] = []

        self.fields['list_terminator'] = 0xBEDEAD

        return

    def get_next_word(self, bytestream):
        new_bytestr = bytestream.read(3)
        return bytestr_to_int(new_bytestr, 3)

    def parse(self, bytestream, fw_id_block_byte_len, next_block_bytestream):
        unpacked_word_list = unpack_memory(bytestream, True, self.memory_type, 'u24')
        self.fields['core_id'] = unpacked_word_list[0]
        self.fields['format_version'] = unpacked_word_list[1]
        self.fields['vendor_id'] = unpacked_word_list[2]
        self.fields['firmware_id'] = unpacked_word_list[3]
        self.fields['firmware_revision'] = unpacked_word_list[4]
        self.fields['sys_config_mem_offsets']['xm_base'] = unpacked_word_list[5]
        self.fields['sys_config_mem_offsets']['xm_size'] = unpacked_word_list[6]
        self.fields['sys_config_mem_offsets']['ym_base'] = unpacked_word_list[7]
        self.fields['sys_config_mem_offsets']['ym_size'] = unpacked_word_list[8]
        self.fields['number_of_algorithms'] = unpacked_word_list[9]

        # Halo wmfw may be generated using the nopad option which may split any data block into a packed block
        # and unpacked block. It is vital the FW-Id table is processed in its entirety. There is no flag in
        # the wmfw to indicate that the nopad option was used.
        # If nopad was used the FW-Id table will likely be split into 2 consecutive blocks.
        # Two checks will be used to decide whether the FW-Id block is split:
        # 1. If FW-id table size (based on the algo count) <= byte len from block header, then likely nopad was NOT used
        # 2.a If 0xBEDEAD is contained inside the 1st block then likely nopad was NOT used
        # 2.b If nopad was used assume 0xBEDEAD to appear in the immediate next block after the first FW-Id block, this
        # block will be UNPACKED data.
        # (For simplicity we're assuming the 0xBEDEAD block will follow immediately after but in theory it  could be anywhere.
        # This will break the tool)

        # In the case nopad was used, the FW-id table size (based on the algo count) means that for odd algo counts
        # the trailing block will contain only 0xBEADED and no other data. Whereas even algo counts mean the trailing
        # block will contain the last 2 words of data (YM-base, YM-size) and the 0xBEDEAD

        algo_cnt = self.fields['number_of_algorithms']
        fw_id_byte_size = (11 + (algo_cnt * 6)) * 3
        if fw_id_byte_size > fw_id_block_byte_len:
            # If the total fw-id table size is larger than this block it is likely it has been
            # split into a following block (due to the nopad option)
            if algo_cnt % 2:
                # If algo cnt is odd, the 2nd block will contain only 0xBEDEAD occupying 4 bytes.
                # Expect 0xBEDEAD and only 0xBEDEAD data in the 2nd block
                next_block_unpacked_word_list = unpack_memory(next_block_bytestream)
                if 0xBEDEAD not in next_block_unpacked_word_list:
                    error_exit("Parsing Halo FW-Id: terminator was expected in 2nd data block but not found")
                if 0xBEDEAD in unpacked_word_list:
                    error_exit("Parsing Halo FW-Id: terminator found in 1st data block instead on 2nd")

                for i in range(0, self.fields['number_of_algorithms']):
                    new_index = 10 + (i * 6)
                    new_info = dict()
                    new_info['algorithm_id'] = unpacked_word_list[new_index]
                    new_info['algorithm_version'] = unpacked_word_list[new_index + 1]
                    new_info['algorithm_offsets'] = dict()
                    new_info['algorithm_offsets']['xm_base'] = unpacked_word_list[new_index + 2]
                    new_info['algorithm_offsets']['xm_size'] = unpacked_word_list[new_index + 3]
                    new_info['algorithm_offsets']['ym_base'] = unpacked_word_list[new_index + 4]
                    new_info['algorithm_offsets']['ym_size'] = unpacked_word_list[new_index + 5]
                    self.fields['algorithm_info'].append(new_info)

            else:
                # parse only n-1 algo's within fw-id table, then parse last algo but do not expect the
                # ym_base, ym_size words to be present. Instead, expect these from the next block
                new_index = 0
                for i in range(0, self.fields['number_of_algorithms'] - 1):
                    new_index = 10 + (i * 6)
                    new_info = dict()
                    new_info['algorithm_id'] = unpacked_word_list[new_index]
                    new_info['algorithm_version'] = unpacked_word_list[new_index + 1]
                    new_info['algorithm_offsets'] = dict()
                    new_info['algorithm_offsets']['xm_base'] = unpacked_word_list[new_index + 2]
                    new_info['algorithm_offsets']['xm_size'] = unpacked_word_list[new_index + 3]
                    new_info['algorithm_offsets']['ym_base'] = unpacked_word_list[new_index + 4]
                    new_info['algorithm_offsets']['ym_size'] = unpacked_word_list[new_index + 5]
                    self.fields['algorithm_info'].append(new_info)

                new_info = dict()
                new_info['algorithm_id'] = unpacked_word_list[new_index + 1]
                new_info['algorithm_version'] = unpacked_word_list[new_index + 2]
                new_info['algorithm_offsets'] = dict()
                new_info['algorithm_offsets']['xm_base'] = unpacked_word_list[new_index + 3]
                new_info['algorithm_offsets']['xm_size'] = unpacked_word_list[new_index + 4]

                # Extract YMBase & YMSize from 2nd block
                next_block_unpacked_word_list = unpack_memory(next_block_bytestream)
                new_info['algorithm_offsets']['ym_base'] = next_block_unpacked_word_list[0]
                new_info['algorithm_offsets']['ym_size'] = next_block_unpacked_word_list[1]
                self.fields['algorithm_info'].append(new_info)

                # Ensure 0xBEDEAD exists in 2nd block and not in the prev one
                if 0xBEDEAD not in next_block_unpacked_word_list:
                    error_exit("Parsing Halo FW-Id: terminator was expected in 2nd data block but not found")
                if 0xBEDEAD in unpacked_word_list:
                    error_exit("Parsing Halo FW-Id: terminator found in 1st data block instead on 2nd")

        else:
            # Else fw-id data is all in the same block
            for i in range(0, self.fields['number_of_algorithms']):
                new_index = 10 + (i * 6)
                new_info = dict()
                new_info['algorithm_id'] = unpacked_word_list[new_index]
                new_info['algorithm_version'] = unpacked_word_list[new_index + 1]
                new_info['algorithm_offsets'] = dict()
                new_info['algorithm_offsets']['xm_base'] = unpacked_word_list[new_index + 2]
                new_info['algorithm_offsets']['xm_size'] = unpacked_word_list[new_index + 3]
                new_info['algorithm_offsets']['ym_base'] = unpacked_word_list[new_index + 4]
                new_info['algorithm_offsets']['ym_size'] = unpacked_word_list[new_index + 5]
                self.fields['algorithm_info'].append(new_info)

            if 0xBEDEAD not in unpacked_word_list:
                error_exit("Parsing Halo FW-Id: terminator expected in 1st data block but not found")

    def get_adjusted_offset(self, algorithm_id, memory_region, start_offset):
        temp_offsets = None
        for alg_info in self.fields['algorithm_info']:
            if (algorithm_id == alg_info['algorithm_id']):
                temp_offsets = alg_info['algorithm_offsets']
        if (temp_offsets == None):
            temp_offsets = self.fields['sys_config_mem_offsets']

        if (memory_region == 'xm'):
            return (start_offset + temp_offsets['xm_base'])
        else:
            return (start_offset + temp_offsets['ym_base'])

    def __str__(self):
        return component_to_string(self, 'Firmware ID Block')

class adsp_firmware_id_block:

    def __init__(self):
        self.fields = dict()
        self.fields['core_id'] = 0
        self.fields['format_version'] = 0
        self.fields['firmware_id'] = 0
        self.fields['firmware_revision'] = 0
        self.fields['sys_config_mem_offsets'] = dict()
        self.fields['sys_config_mem_offsets']['xm_base'] = 0
        self.fields['sys_config_mem_offsets']['xm_size'] = 0
        self.fields['sys_config_mem_offsets']['ym_base'] = 0
        self.fields['sys_config_mem_offsets']['ym_size'] = 0
        self.fields['number_of_algorithms'] = 0
        self.fields['algorithm_info'] = []

        self.fields['list_terminator'] = 0xBEDEAD

        return

    def get_next_word(self, bytestream):
        new_bytestr = bytestream.read(3)
        return bytestr_to_int(new_bytestr, 3)

    def parse(self, bytestream):
        unpacked_word_list = unpack_memory(bytestream)
        self.fields['core_id'] = int.from_bytes(unpacked_word_list[0].to_bytes(4, byteorder='little'), byteorder='big', signed=False)
        self.fields['format_version'] = int.from_bytes(unpacked_word_list[1].to_bytes(4, byteorder='little'), byteorder='big', signed=False)
        self.fields['firmware_id'] = unpacked_word_list[2]
        self.fields['firmware_revision'] = unpacked_word_list[3]
        self.fields['sys_config_mem_offsets']['zm_base'] = unpacked_word_list[4]
        self.fields['sys_config_mem_offsets']['xm_base'] = unpacked_word_list[5]
        self.fields['sys_config_mem_offsets']['ym_base'] = unpacked_word_list[6]
        self.fields['number_of_algorithms'] = unpacked_word_list[7]
        for i in range(0, self.fields['number_of_algorithms']):
            new_index = 8 + (i * 5)
            new_info = dict()
            new_info['algorithm_id'] = unpacked_word_list[new_index]
            new_info['algorithm_version'] = unpacked_word_list[new_index + 1]
            new_info['algorithm_offsets'] = dict()
            new_info['algorithm_offsets']['zm_base'] = unpacked_word_list[new_index + 2]
            new_info['algorithm_offsets']['xm_base'] = unpacked_word_list[new_index + 3]
            new_info['algorithm_offsets']['ym_base'] = unpacked_word_list[new_index + 4]
            self.fields['algorithm_info'].append(new_info)

        return

    def get_adjusted_offset(self, algorithm_id, memory_region, start_offset):
        temp_offsets = None
        for alg_info in self.fields['algorithm_info']:
            if (algorithm_id == alg_info['algorithm_id']):
                temp_offsets = alg_info['algorithm_offsets']
        if (temp_offsets == None):
            temp_offsets = self.fields['sys_config_mem_offsets']

        if (memory_region == 'zm'):
            return (start_offset + temp_offsets['zm_base'])
        elif (memory_region == 'xm'):
            return (start_offset + temp_offsets['xm_base'])
        else:
            return (start_offset + temp_offsets['ym_base'])

    def __str__(self):
        return component_to_string(self, 'Firmware ID Block')

class wmfw_parser:

    def __init__(self, filename):
        self.filename = filename
        self.blocks = []
        self.file_checksum = 0

        return

    def parse(self):
        f = open(self.filename, 'rb')

        # Make header
        self.header = wmfw_header(f)
        self.header.parse(f)

        if self.header.fields['file_format_version'] in [1, 2]: # If ADSP2 v1 or v2
            self.block_types = adsp_memory_region_block_types
        else:  # Else Halo
            self.block_types = halo_memory_region_block_types

        # Get blocks
        while (not f.tell() == os.fstat(f.fileno()).st_size):
            temp_block_type = file_int_peek(f, 4)
            temp_block_type = bytes_from_word(temp_block_type, 3, 1)
            self.blocks.append(self.block_factory(f, temp_block_type, self.header.fields['file_format_version']))
            self.blocks[-1].parse(f)

        # Get Firmware ID Block for beginning XM - assume it's in first data block
        first_data_block_bytes = self.blocks[1].data
        first_data_block_memory_type = self.blocks[1].memory_type
        block_bytestream = io.BytesIO(b''.join(first_data_block_bytes))
        if self.header.fields['file_format_version'] in [1, 2]: # If ADSP2 v1 or v2== 2
            self.fw_id_block = adsp_firmware_id_block()
            self.fw_id_block.parse(block_bytestream)
        else:  # Else Halo
            self.fw_id_block = halo_firmware_id_block(first_data_block_memory_type)

            # Halo fw-id block parser may require the 2nd data block incase the fw-id table is split
            next_data_block_bytes = self.blocks[2].data
            next_block_bytestream = io.BytesIO(b''.join(next_data_block_bytes))
            self.fw_id_block.parse(block_bytestream,
                                   self.blocks[1].fields['block_size'],
                                   next_block_bytestream)

        return

    def validate_checksum(self):
        self.file_checksum = self.header.sum_bytes()

        return

    def block_factory(self, file, block_type, file_format_version):
        if (block_type in self.block_types):
            return wmfw_memory_region_data_block(file)
        elif (block_type == informational_text_block_type):
            return wmfw_informational_text_data_block(file)
        elif (block_type == algorithm_information_data_block):
            return wmfw_algorithm_information_data_block(file, file_format_version)
        else:
            return wmfw_block(file)

    def get_data_blocks(self):
        temp_blocks = []
        for block in self.blocks:
            if (block.fields['type'] in self.block_types):
                temp_blocks.append(block)
        return temp_blocks

    def get_algorithm_information_data_blocks(self):
        temp_blocks = []
        for block in self.blocks:
            if (block.fields['type'] == algorithm_information_data_block):
                temp_blocks.append(block)
        return temp_blocks

    def __str__(self):
        output_str = str(self.header) + "\n"
        output_str = output_str + str(self.fw_id_block) + "\n"
        for block in self.blocks:
            output_str = output_str + str(block) + "\n"

        return output_str

class wmfw_rom(wmfw_parser):

    def __init__(self, part_number):
        self.part_number = part_number
        wmfw_parser.__init__(self, "foo")
        return

    def update_fw_id_block_cs40l26(self):
        self.fw_id_block.fields['core_id'] = 0x40000
        self.fw_id_block.fields['format_version'] = 0x30000
        self.fw_id_block.fields['vendor_id'] = 0x2
        self.fw_id_block.fields['firmware_id'] = 0x0
        self.fw_id_block.fields['firmware_revision'] = 0x12345
        self.fw_id_block.fields['sys_config_mem_offsets']['xm_base'] = 0x3ea
        self.fw_id_block.fields['sys_config_mem_offsets']['xm_size'] = 0x16
        self.fw_id_block.fields['sys_config_mem_offsets']['ym_base'] = 0x57f
        self.fw_id_block.fields['sys_config_mem_offsets']['ym_size'] = 0x1
        self.fw_id_block.fields['number_of_algorithms'] = 0x1c
        self.fw_id_block.fields['algorithm_info'] = [
            {'algorithm_id': 258567, 'algorithm_version': 65539, 'algorithm_offsets': {'xm_base': 1884, 'xm_size': 238, 'ym_base': 4, 'ym_size': 238}},{'algorithm_id': 196797, 'algorithm_version': 131074, 'algorithm_offsets': {'xm_base': 2122, 'xm_size': 4013, 'ym_base': 1970, 'ym_size': 1}},
            {'algorithm_id': 258560, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7150, 'xm_size': 20, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258561, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7241, 'xm_size': 15, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258562, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7170, 'xm_size': 20, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258563, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7423, 'xm_size': 5, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258572, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6460, 'xm_size': 6, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258566, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 212, 'xm_size': 210, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258587, 'algorithm_version': 65537, 'algorithm_offsets': {'xm_base': 978, 'xm_size': 24, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258578, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 1036, 'xm_size': 30, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258575, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 776, 'xm_size': 12, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258577, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 2566, 'xm_size': 250, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 196880, 'algorithm_version': 262146, 'algorithm_offsets': {'xm_base': 7034, 'xm_size': 60, 'ym_base': 1024, 'ym_size': 18}},
            {'algorithm_id': 258576, 'algorithm_version': 327681, 'algorithm_offsets': {'xm_base': 2922, 'xm_size': 1410, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258579, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 5327, 'xm_size': 53, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258580, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 5594, 'xm_size': 164, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258574, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 5759, 'xm_size': 47, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258581, 'algorithm_version': 66048, 'algorithm_offsets': {'xm_base': 5806, 'xm_size': 148, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258582, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6033, 'xm_size': 47, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258583, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6133, 'xm_size': 46, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258584, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6256, 'xm_size': 41, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258585, 'algorithm_version': 65793, 'algorithm_offsets': {'xm_base': 6302, 'xm_size': 128, 'ym_base': 0, 'ym_size': 1}},
            {'algorithm_id': 258586, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6442, 'xm_size': 256, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 204815, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 6758, 'xm_size': 161, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258588, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 6919, 'xm_size': 49, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 196770, 'algorithm_version': 851968, 'algorithm_offsets': {'xm_base': 6968, 'xm_size': 52, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258589, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7077, 'xm_size': 22, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 258591, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 7099, 'xm_size': 17, 'ym_base': 0, 'ym_size': 0}}
        ]
        self.fw_id_block.fields['list_terminator'] = 0xbedead
        return

    def update_fw_id_block_cs40l50(self):
        self.fw_id_block.fields['core_id'] = 0x40000
        self.fw_id_block.fields['format_version'] = 0x30000
        self.fw_id_block.fields['vendor_id'] = 0x2
        self.fw_id_block.fields['firmware_id'] = 0x0
        self.fw_id_block.fields['firmware_revision'] = 0x12345
        self.fw_id_block.fields['sys_config_mem_offsets']['xm_base'] = 0x878
        self.fw_id_block.fields['sys_config_mem_offsets']['xm_size'] = 0x22
        self.fw_id_block.fields['sys_config_mem_offsets']['ym_base'] = 0x18e
        self.fw_id_block.fields['sys_config_mem_offsets']['ym_size'] = 0x1
        self.fw_id_block.fields['number_of_algorithms'] = 0x2c
        self.fw_id_block.fields['algorithm_info'] = [{'algorithm_id': 651783, 'algorithm_version': 131585, 'algorithm_offsets': {'xm_base': 0, 'xm_size': 0, 'ym_base': 1028, 'ym_size': 250}},
            {'algorithm_id': 590013, 'algorithm_version': 327937, 'algorithm_offsets': {'xm_base': 5888, 'xm_size': 2018, 'ym_base': 3072, 'ym_size': 2000}},
            {'algorithm_id': 590141, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 3322, 'xm_size': 74, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651776, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 4173, 'xm_size': 20, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651777, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 4194, 'xm_size': 45, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651778, 'algorithm_version': 131585, 'algorithm_offsets': {'xm_base': 2536, 'xm_size': 23, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651779, 'algorithm_version': 65799, 'algorithm_offsets': {'xm_base': 4272, 'xm_size': 5, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651788, 'algorithm_version': 196610, 'algorithm_offsets': {'xm_base': 3034, 'xm_size': 9, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651782, 'algorithm_version': 131072, 'algorithm_offsets': {'xm_base': 4282, 'xm_size': 280, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651803, 'algorithm_version': 131329, 'algorithm_offsets': {'xm_base': 3043, 'xm_size': 65, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 590144, 'algorithm_version': 65538, 'algorithm_offsets': {'xm_base': 3108, 'xm_size': 10, 'ym_base': 1284, 'ym_size': 6}},
            {'algorithm_id': 651794, 'algorithm_version': 720899, 'algorithm_offsets': {'xm_base': 2572, 'xm_size': 36, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651791, 'algorithm_version': 65538, 'algorithm_offsets': {'xm_base': 5416, 'xm_size': 12, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651793, 'algorithm_version': 131072, 'algorithm_offsets': {'xm_base': 1844, 'xm_size': 252, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651792, 'algorithm_version': 851968, 'algorithm_offsets': {'xm_base': 776, 'xm_size': 835, 'ym_base': 1574, 'ym_size': 89}},
            {'algorithm_id': 651795, 'algorithm_version': 131072, 'algorithm_offsets': {'xm_base': 3431, 'xm_size': 108, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651796, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3539, 'xm_size': 165, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651790, 'algorithm_version': 196864, 'algorithm_offsets': {'xm_base': 3704, 'xm_size': 56, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651797, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3200, 'xm_size': 139, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651798, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3760, 'xm_size': 48, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651799, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3808, 'xm_size': 48, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651800, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3856, 'xm_size': 43, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651801, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3354, 'xm_size': 77, 'ym_base': 0, 'ym_size': 1}},
            {'algorithm_id': 651802, 'algorithm_version': 327936, 'algorithm_offsets': {'xm_base': 514, 'xm_size': 262, 'ym_base': 1536, 'ym_size': 8}},
            {'algorithm_id': 598031, 'algorithm_version': 393216, 'algorithm_offsets': {'xm_base': 332, 'xm_size': 130, 'ym_base': 1544, 'ym_size': 30}},
            {'algorithm_id': 651804, 'algorithm_version': 262144, 'algorithm_offsets': {'xm_base': 2096, 'xm_size': 21, 'ym_base': 2033, 'ym_size': 28}},
            {'algorithm_id': 589986, 'algorithm_version': 851968, 'algorithm_offsets': {'xm_base': 2891, 'xm_size': 52, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651805, 'algorithm_version': 66048, 'algorithm_offsets': {'xm_base': 3006, 'xm_size': 25, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651807, 'algorithm_version': 65792, 'algorithm_offsets': {'xm_base': 3031, 'xm_size': 17, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651809, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3059, 'xm_size': 24, 'ym_base': 140, 'ym_size': 256}},
            {'algorithm_id': 651808, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 3340, 'xm_size': 13, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 590140, 'algorithm_version': 393216, 'algorithm_offsets': {'xm_base': 3118, 'xm_size': 11, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651812, 'algorithm_version': 196609, 'algorithm_offsets': {'xm_base': 2117, 'xm_size': 12, 'ym_base': 2061, 'ym_size': 2}},
            {'algorithm_id': 651814, 'algorithm_version': 65538, 'algorithm_offsets': {'xm_base': 2280, 'xm_size': 82, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651815, 'algorithm_version': 196608, 'algorithm_offsets': {'xm_base': 1611, 'xm_size': 5, 'ym_base': 1663, 'ym_size': 3}},
            {'algorithm_id': 651816, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 2660, 'xm_size': 20, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651810, 'algorithm_version': 65537, 'algorithm_offsets': {'xm_base': 0, 'xm_size': 0, 'ym_base': 0, 'ym_size': 396}},
            {'algorithm_id': 651817, 'algorithm_version': 65538, 'algorithm_offsets': {'xm_base': 3154, 'xm_size': 4, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651811, 'algorithm_version': 65539, 'algorithm_offsets': {'xm_base': 5466, 'xm_size': 11, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651818, 'algorithm_version': 66050, 'algorithm_offsets': {'xm_base': 3112, 'xm_size': 39, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651819, 'algorithm_version': 1, 'algorithm_offsets': {'xm_base': 2129, 'xm_size': 8, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 590146, 'algorithm_version': 65795, 'algorithm_offsets': {'xm_base': 3161, 'xm_size': 10, 'ym_base': 0, 'ym_size': 0}},
            {'algorithm_id': 651821, 'algorithm_version': 65536, 'algorithm_offsets': {'xm_base': 1616, 'xm_size': 228, 'ym_base': 1666, 'ym_size': 56}},
            {'algorithm_id': 651820, 'algorithm_version': 65537, 'algorithm_offsets': {'xm_base': 5512, 'xm_size': 54, 'ym_base': 0, 'ym_size': 0}}
        ]
        self.fw_id_block.fields['list_terminator'] = 0xbedead
        return

    def parse(self):
        self.fw_id_block = halo_firmware_id_block(None)
        if (self.part_number == 'cs40l26'):
            self.update_fw_id_block_cs40l26()
        elif (self.part_number == 'cs40l50'):
            self.update_fw_id_block_cs40l50()
        else:
            error_exit('Unsupported ROM: ' + self.part_number)

        return

    def __str__(self):
        output_str = str(self.fw_id_block) + "\n"
        for block in self.blocks:
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
        # Check that WMFW path exists
        if (os.path.exists(args[1])):
            result = True

    return result

def print_start():
    print("")
    print("wmfw_parser")
    print("Parser for WMFW files")
    print("Version " + VERSION_STRING)

    return

def print_usage():
    print("")
    print("Usage:")
    print("    python wmfw_parser.py <wmfw_path>")

    return

def print_args(args):
    print("")
    print("WMFW Path: " + args[1])

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

def get_memory_region_from_type(type):
    ret = None
    if (type in halo_block_types_memory_region_xm):
        ret = 'xm'
    elif (type in halo_block_types_memory_region_ym):
        ret = 'ym'
    elif (type == adsp_zm_u24_block_type):
        ret = 'zm'
    elif (type in halo_block_types_memory_region_pm):
        ret = 'pm'
    elif (type == adsp_pm_p32_block_type):
        ret = 'pm'
    elif (type == (absolute_addressing_data_block_type << 8)):
        ret = 'abs'

    return ret

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

    p = wmfw_parser(argv[1])
    p.parse()
    p.validate_checksum()

    print_results(str(p))

    print_end()

    return

if __name__ == "__main__":
    main(sys.argv)
