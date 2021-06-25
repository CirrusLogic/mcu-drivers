# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Storage and Interface classes for vregmap_generator
# File    : vregmap_classes.py
# --------------------------------------------------------------------------
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
# --------------------------------------------------------------------------
#
# Environment Requirements: None
#
# ==========================================================================

# ==========================================================================
# IMPORTS
# ==========================================================================
import abc

# ==========================================================================
# VERSION
# ==========================================================================

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================

# ==========================================================================
# CLASSES
# ==========================================================================
class vregmap_bitfield:
    def __init__(self, name, size, shift, is_writeable):
        self.name = name
        self.size = size
        self.shift = shift
        self.mask = ((2 ** self.size) - 1) << self.shift
        self.is_writeable = is_writeable
        return

    def __str__(self):
        output_str = "Bitfield:\n"
        output_str += "    name: " + self.name + "\n"
        output_str += "    mask: " + hex(self.mask) + "\n"
        output_str += "    size: " + hex(self.size) + "\n"
        output_str += "    shift: " + hex(self.shift) + "\n"
        output_str += "    RO: " + str(not self.is_writeable) + "\n"
        return output_str

class vregmap_register:
    def __init__(self, name, address, default, is_writeable):
        self.name = name
        self.address = address
        self.default = default
        self.is_writeable = is_writeable
        self.default_bitfield = vregmap_bitfield(self.name, 32, 0, self.is_writeable)
        self.bitfields = []
        return

    def add_bitfield(self, name, size, shift, is_writeable):
        # Check for access
        if ((not self.is_writeable) and (is_writeable)):
            print("Warning:  Access incompatibility for: " + self.name + "." + name)

        self.bitfields.append(vregmap_bitfield(name, size, shift, is_writeable))
        return

    def __str__(self):
        output_str = "Register:\n"
        output_str += "    name: " + self.name + "\n"
        output_str += "    address: " + hex(self.address) + "\n"
        output_str += "    default: " + hex(self.default) + "\n"
        output_str += "    RO: " + str(not self.is_writeable) + "\n"
        output_str += "\n"
        if (len(self.bitfields) <= 0):
            output_str += str(self.default_bitfield) + "\n"
        else:
            for b in self.bitfields:
                output_str += str(b) + "\n"
        return output_str

class vregmap_device:
    def __init__(self, device_id_type, reported_id):
        self.device_id_type = device_id_type
        self.reported_id = reported_id
        self.registers = []

        return

    def __str__(self):
        output_str = "Device:\n"
        output_str += "    device_id_type: " + self.device_id_type + "\n"
        output_str += "    reported_id: " + self.reported_id + "\n"
        output_str += "\n"
        for r in self.registers:
            output_str += str(r)

        return output_str

class vregmap_importer_interface(metaclass=abc.ABCMeta):
    @classmethod
    def __subclasshook__(cls, subclass):
        return (hasattr(subclass, 'get_device') and
                callable(subclass.get_device) or
                NotImplemented)

    @abc.abstractmethod
    def get_device(self):
        """Return list of all registers"""
        raise NotImplementedError

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
