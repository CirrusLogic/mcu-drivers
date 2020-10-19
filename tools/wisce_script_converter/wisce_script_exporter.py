#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Abstract Base Class for exporters for wisce_scripts
# File    : wisce_script_exporter.py
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
from abc import ABC, abstractmethod

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class wisce_script_exporter(ABC):
    def __init__(self, attributes):
        self.attributes = attributes

        super().__init__()
        return

    @abstractmethod
    def add_transaction(self, transaction): pass

    @abstractmethod
    def add_metadata_text_line(self, line): pass

    @abstractmethod
    def to_file(self): pass

    @abstractmethod
    def __str__(self): pass

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

