#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Template for WISCE Script files
# File    : wisce_file_templates.py
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
import string
from firmware_exporter import firmware_exporter

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
wisce_script_template_str = """
*************************************************************************************
* Cirrus Logic {part_number_uc} Firmware and Tuning Boot Script.                    *
*                                                                                   *
*************************************************************************************

{metadata_text}

    message Script - Starting {part_number_uc} Firmware and Tuning Boot Script.

{wisce_script_block_writes}

    message Script - Finished {part_number_uc} Firmware and Tuning Boot Script.

*** end of file

"""

wisce_script_block_str = """
BLOCK_WRITE {block_address} SMbus_32inx_32dat {i2c_address}
{block_words}
END
"""

#==========================================================================
# CLASSES
#==========================================================================
class wisce_script_file(firmware_exporter):
    def __init__(self, attributes):
        firmware_exporter.__init__(self, attributes)
        self.template_str = wisce_script_template_str
        self.output_str = ''
        self.terms = dict()
        self.terms['part_number_lc'] = self.attributes['part_number_str'].lower()
        self.terms['part_number_uc'] = self.attributes['part_number_str'].upper()
        self.terms['wisce_blocks'] = ''
        self.terms['i2c_address'] = self.attributes['i2c_address']
        self.terms['metadata_text'] = ''
        self.block_write_32dat_per_line = 8
        return

    def create_block_string(self, data_bytes):
        byte_count = 0
        word_count = 0
        temp_data_str = ''
        temp_byte_str = ''
        for data_byte in data_bytes:
            temp_byte_str = "{0:0{1}X}".format(data_byte, 2)
            temp_data_str = temp_data_str + temp_byte_str
            byte_count = byte_count + 1
            if ((byte_count % 4) == 0):
                temp_data_str = temp_data_str + ' '
                byte_count = 0

                word_count = word_count + 1
                if (((word_count % self.block_write_32dat_per_line) == 0) and (word_count < len(data_bytes)/4)):
                    temp_data_str = temp_data_str + "\n"

        return temp_data_str

    def update_block_info(self, fw_block_total, coeff_block_totals): pass
    def add_control(self, algorithm_name, algorithm_id, control_name, address): pass

    def add_data_block(self, address, data_bytes):
        # Create string for block data
        address_str =  "0x" + "{0:0{1}X}".format(address, 8)
        temp_str = wisce_script_block_str.replace('{block_address}', address_str)
        temp_str = temp_str.replace('{block_words}', self.create_block_string(data_bytes))

        self.terms['wisce_blocks'] = self.terms['wisce_blocks'] + temp_str + '\n'

        return

    def add_fw_block(self, address, data_bytes):
        return self.add_data_block(address, data_bytes)

    def add_coeff_block(self, index, address, data_bytes):
        return self.add_data_block(address, data_bytes)

    def add_metadata_text_line(self, line):
        self.terms['metadata_text'] = self.terms['metadata_text'] + '* ' + line + '\n'
        return

    def __str__(self):
        output_str = wisce_script_template_str

        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        output_str = output_str.replace('{wisce_script_block_writes}', self.terms['wisce_blocks'])

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('{i2c_address}', self.terms['i2c_address'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str


    def to_file(self):
        temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_wisce_script_output.txt"
        results_str = "Exported to " + temp_filename + "\n"

        f = open(temp_filename, 'w')
        f.write(str(self))
        f.close()

        return results_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

