#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Exporter for JSON file format
# File    : json_exporter.py
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
import json
import datetime

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class json_exporter(firmware_exporter):
    def __init__(self, attributes):
        firmware_exporter.__init__(self, attributes)

        self.fw_export = dict()

        self.fw_export['tool_info'] = dict()
        self.fw_export['tool_info']['timestamp'] = '{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.now())
        self.fw_export['tool_info']['metadata_text'] = []

        self.fw_export['fw_info'] = self.attributes['fw_meta']
        for key in  self.fw_export['fw_info'].keys():
            if (isinstance(self.fw_export['fw_info'][key], int)):
                self.fw_export['fw_info'][key] = hex(self.fw_export['fw_info'][key])

        self.fw_export['controls'] = dict()

        return

    def update_block_info(self, fw_block_total, coeff_block_totals):
        return

    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        if (algorithm_name not in self.fw_export['controls']):
            self.fw_export['controls'][algorithm_name] = dict()
            self.fw_export['controls'][algorithm_name]['id'] = hex(algorithm_id)
            self.fw_export['controls'][algorithm_name]['controls'] = dict()

        self.fw_export['controls'][algorithm_name]['controls'][control_name] = hex(address)

        return

    def add_metadata_text_line(self, line):
        self.fw_export['tool_info']['metadata_text'].append(line)
        return

    def add_fw_block(self, address, data_bytes):
        return

    def add_coeff_block(self, index, address, data_bytes):
        return

    def to_file(self):
        results_str = 'Exported to files:\n'

        json_filename = self.attributes['part_number_str'] + '.json'
        try:
            f = open(json_filename, 'w')
            #f.write(json.dumps(self.fw_export))
            f.write(json.dumps(self.fw_export, sort_keys=False, indent=4))

            results_str += json_filename + '\n'
        except:
            error_exit('Failure writing output file!')

        return results_str

    def __str__(self):
        return "WMFW Info:\n" + json.dumps(self.fw_export, sort_keys=True, indent=4)

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

