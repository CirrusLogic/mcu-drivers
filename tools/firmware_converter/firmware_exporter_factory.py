#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Firmware Exporter Factory class
# File    : firmware_exporter_factory.py
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
from firmware_exporter import firmware_exporter
from c_h_file_templates import source_file_exporter
from wisce_file_templates import wisce_script_file
from fw_img_v1_templates import fw_img_v1_file

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
exporter_types = ['c_array', 'fw_img_v1', 'wisce']

#==========================================================================
# CLASSES
#==========================================================================
class firmware_exporter_factory(firmware_exporter):

    def __init__(self, attributes):
        firmware_exporter.__init__(self, attributes)
        self.exporters = []
        self.attributes = attributes
        return
        
    def add_firmware_exporter(self, type):
        if (type not in exporter_types):
            exit(1)
            
        if (type == 'c_array'):
            e = source_file_exporter(self.attributes)
            self.exporters.append(e)
        elif (type == 'fw_img_v1'):
            e = fw_img_v1_file(self.attributes)
            self.exporters.append(e)
        elif (type == 'wisce'):
            e = wisce_script_file(self.attributes)
            self.exporters.append(e)
        else:
            print('Unknown firmware exporter type!')
            exit(1)
        
        return
        
    def update_block_info(self, fw_block_total, coeff_block_totals):
        for e in self.exporters:
            e.update_block_info(fw_block_total, coeff_block_totals)
            
        return
        
    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        for e in self.exporters:
            e.add_control(algorithm_name, algorithm_id, control_name, address)
            
        return
        
    def add_metadata_text_line(self, line):
        for e in self.exporters:
            e.add_metadata_text_line(line)
            
        return
        
    def add_fw_block(self, address, data_bytes):
        for e in self.exporters:
            e.add_fw_block(address, data_bytes)
            
        return
        
    def add_coeff_block(self, index, address, data_bytes):
        for e in self.exporters:
            e.add_coeff_block(index, address, data_bytes)
            
        return
        
    def __str__(self):
        output_str = ''
        
        for e in self.exporters:
            output_str = output_str + str(e)
            
        return output_str
        
    def to_file(self):
        results_str = ''
        for e in self.exporters:
            results_str = results_str + e.to_file()
        
        return results_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

