#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Template for WISCE Script files
# File    : wisce_file_templates.py
#--------------------------------------------------------------------------
# Redistribution and use of this file in source and binary forms, with
# or without modification, are permitted.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#--------------------------------------------------------------------------
#
# Environment Requirements: None
#
#==========================================================================

#==========================================================================
# IMPORTS
#==========================================================================
import string

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
wisce_script_template_str = """
*************************************************************************************
* Cirrus Logic {part_number_uc} Firmware and Tuning Boot Script.                    *
*                                                                                   *
*************************************************************************************

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
class wisce_script_file:
    def __init__(self, part_number_str, i2c_address):
        self.template_str = wisce_script_template_str
        self.output_str = ''
        self.terms = dict()
        self.terms['part_number_lc'] = part_number_str.lower()
        self.terms['part_number_uc'] = part_number_str.upper()
        self.terms['wisce_blocks'] = ''
        self.terms['i2c_address'] = i2c_address
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

    def add_data_block(self, address, data_bytes):
        # Create string for block data
        address_str =  "0x" + "{0:0{1}X}".format(address, 8)
        temp_str = wisce_script_block_str.replace('{block_address}', address_str)
        temp_str = temp_str.replace('{block_words}', self.create_block_string(data_bytes))

        self.terms['wisce_blocks'] = self.terms['wisce_blocks'] + temp_str + '\n'

        return

    def __str__(self):
        output_str = wisce_script_template_str
        output_str = output_str.replace('{wisce_script_block_writes}', self.terms['wisce_blocks'])

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('{i2c_address}', self.terms['i2c_address'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str
#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

