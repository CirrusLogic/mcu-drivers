# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Import SCS Device XML to list of vregmap registers and bitfields
# File    : vregmap_scs_xml_importer.py
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
import vregmap_classes
import xml.etree.ElementTree as ET

# ==========================================================================
# VERSION
# ==========================================================================

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================

# ==========================================================================
# CLASSES
# ==========================================================================
class vregmap_scs_xml_importer(vregmap_classes.vregmap_importer_interface):
    def __init__(self, filename):
        self.filename = filename
        self.deviceId = ""
        self.reportedId = ""
        self.device = None

        tempXmlP = ET.XMLParser(encoding="utf-8")
        tempTree = ET.parse(self.filename, parser=tempXmlP)
        tempRoot = tempTree.getroot()

        for child in tempRoot:
            if (child.tag == "DeviceInfo"):
                self.device = self.get_device_info(child)
            if (child.tag == "Registers"):
                registers = []
                for reg_xml in child:
                    registers.append(self.get_register_info(reg_xml))
                self.device.registers = registers.copy()
            if (child.tag == "Fields"):
                regname = ""
                bitfield = None
                for reg_xml in child:
                    (regname, bitfield) = self.get_bitfield_info(reg_xml)
                    for r in self.device.registers:
                        if (regname == r.name):
                            r.bitfields.append(bitfield)

        return

    def get_device_info(self, xml):
        for child in xml:
            if (child.tag == "Type"):
                self.deviceId = child.text
            elif (child.tag == "ReportedID"):
                self.reportedId = child.text
            elif ((child.tag == "RegisterBitWidth") and (child.text != "32")):
                print("Warning: 'RegisterBitWidth' is not 32!")

        return vregmap_classes.vregmap_device(self.deviceId, self.reportedId)

    def get_register_info(self, xml):
        address = 0xFFFFFFFF
        name = ""
        default = 0x00000000
        is_writeable = False
        bitfields = []
        for child in xml:
            if (child.tag == "Name"):
                name = child.text.upper().replace(' ', '_')
            elif (child.tag == "Address"):
                address = int(child.text, 0)
            elif (child.tag == "DefaultValue"):
                default = int(child.text, 0)
            elif (child.tag == "Access"):
                if (child.text == "R/W"):
                    is_writeable = True

        r = vregmap_classes.vregmap_register(name, address, default, is_writeable)
        return r

    def get_bitfield_info(self, xml):
        name = ""
        is_writeable = False
        top = 0
        bottom = 0
        for child in xml:
            if (child.tag == "Name"):
                name = child.text.upper().replace(' ', '_')
            elif (child.tag == "Address"):
                regname = child.text.split('[')[0]
                top = int(child.text.split('[')[1].split(':')[0])
                bottom = int(child.text.split('[')[1].split(':')[1].split(']')[0])
            elif (child.tag == "Access"):
                if (child.text == "R/W"):
                    is_writeable = True

        return (regname, vregmap_classes.vregmap_bitfield(name, (top - bottom + 1), bottom, is_writeable))

    def get_device(self):
        return self.device

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
