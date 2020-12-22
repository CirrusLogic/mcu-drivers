#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Firmware Exporter Factory class
# File    : firmware_exporter_factory.py
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
from firmware_exporter import firmware_exporter
from c_h_file_templates import source_file_exporter
from wisce_file_templates import wisce_script_file
from fw_img_v1_templates import fw_img_v1_file
from json_exporter import json_exporter

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
exporter_types = ['c_array', 'fw_img_v1', 'fw_img_v2', 'wisce', 'json']

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
            e = fw_img_v1_file(self.attributes, 0x1)
            self.exporters.append(e)
        elif (type == 'fw_img_v2'):
            e = fw_img_v1_file(self.attributes, 0x2)
            self.exporters.append(e)
        elif (type == 'wisce'):
            e = wisce_script_file(self.attributes)
            self.exporters.append(e)
        elif (type == 'json'):
            e = json_exporter(self.attributes)
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

