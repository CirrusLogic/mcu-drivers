#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : WISCE Script Control Port Transaction class
# File    : wisce_script_transaction.py
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

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================

#==========================================================================
# CLASSES
#==========================================================================
class wisce_script_transaction:
    def __init__(self, address, value, comment=None):
        self.__set_address(address)
        self.__set_value(value)
        self.__set_comment(comment)
        return

    def __get_address(self):
        return self.__address

    def __set_address(self, address):
        self.__address = address

    address = property(__get_address, __set_address)

    def __get_value(self):
        return self.__value

    def __set_value(self, value):
        self.__value = value

    value = property(__get_value, __set_value)

    def __get_comment(self):
        return self.__comment

    def __set_comment(self, comment):
        self.__comment = comment

    comment = property(__get_comment, __set_comment)

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

