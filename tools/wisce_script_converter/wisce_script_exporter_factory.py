#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : WISCE Script Exporter Factory class
# File    : wisce_script_exporter_factory.py
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
from wisce_script_exporter import wisce_script_exporter
from c_array_exporter import c_array_exporter

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
exporter_types = ['c_array']

#==========================================================================
# CLASSES
#==========================================================================
class wisce_script_exporter_factory(wisce_script_exporter):

    def __init__(self, attributes):
        wisce_script_exporter.__init__(self, attributes)
        self.exporters = []
        self.attributes = attributes
        return

    def add_exporter(self, type):
        if (type not in exporter_types):
            exit(1)

        if (type == 'c_array'):
            e = c_array_exporter(self.attributes)
            self.exporters.append(e)
        else:
            print('Unknown firmware exporter type!')
            exit(1)

        return

    def add_transaction(self, transaction):
        for e in self.exporters:
            e.add_transaction(transaction)

        return

    def add_metadata_text_line(self, line):
        for e in self.exporters:
            e.add_metadata_text_line(line)

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

