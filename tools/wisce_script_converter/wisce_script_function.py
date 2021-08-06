#==========================================================================
# (c) 2021 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : WISCE Script Control Port Transaction class
# File    : wisce_script_function.py
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
class wisce_script_function:
    def __init__(self, cmd, params, comment=None, length=None):
        self.__set_cmd(cmd)
        self.__set_params(params)
        self.__set_comment(comment)
        self.__set_length(length)
        return

    def __get_cmd(self):
        return self.__cmd

    def __set_cmd(self, cmd):
        self.__cmd = cmd

    cmd = property(__get_cmd, __set_cmd)

    def __get_params(self):
        return self.__params

    def __set_params(self, params):
        self.__params = params

    params = property(__get_params, __set_params)

    def __get_comment(self):
        return self.__comment

    def __set_comment(self, comment):
        self.__comment = comment

    comment = property(__get_comment, __set_comment)

    def __get_length(self):
        return self.__length

    def __set_length(self, length):
        self.__length = length

    length = property(__get_length, __set_length)

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

