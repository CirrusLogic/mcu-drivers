# ==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Import WISCE Device XML to list of vregmap registers and bitfields
# File    : vregmap_wisce_xml_importer.py
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
class vregmap_wisce_xml_importer(vregmap_classes.vregmap_importer_interface):
    def __init__(self, filename):
        self.filename = filename
        self.deviceId = ""
        self.reportedId = ""
        self.device = None

        tempXmlP = ET.XMLParser(encoding="utf-8")
        tempTree = ET.parse(self.filename, parser=tempXmlP)
        tempRoot = tempTree.getroot()

        registers = []
        for child in tempRoot:
            if (child.tag == "DeviceInfo"):
                self.device = self.get_device_info(child)
            if (child.tag == "Registers"):
                for reg_xml in child:
                    registers.append(self.get_register_info(reg_xml))

        self.device.registers = registers.copy()

        return

    def get_device_info(self, xml):
        for child in xml:
            if (child.tag == "DeviceID"):
                self.deviceId = child.text
            elif (child.tag == "ReportedID"):
                self.reportedId = child.text
            elif ((child.tag == "RegisterBits") and (child.text != "32")):
                print("Warning: 'RegisterBits' is not 32!")

        return vregmap_classes.vregmap_device(self.deviceId, self.reportedId)

    def get_register_info(self, xml):
        address = 0xFFFFFFFF
        name = ""
        default = 0x00000000
        is_writeable = False
        bitfields = []
        for child in xml:
            if (child.tag == "Address"):
                address = int(child.text, 0)
            elif (child.tag == "Name"):
                name = child.text.upper().replace(' ', '_')
            elif (child.tag == "Default"):
                default = int(child.text, 0)
            elif (child.tag == "Access"):
                for c in child:
                    if (c.tag == "Write"):
                        is_writeable = True
            elif (child.tag == "BitField"):
                bitfields.append(self.get_bitfield_info(child))

        r = vregmap_classes.vregmap_register(name, address, default, is_writeable)
        r.bitfields = bitfields.copy()
        return r

    def get_bitfield_info(self, xml):
        name = ""
        is_writeable = False
        top = int(xml.attrib['top'])
        width = int(xml.attrib['width'])
        for child in xml:
            if (child.tag == "Name"):
                name = child.text.upper().replace(' ', '_')
            elif (child.tag == "Access"):
                for a in child:
                    if (a.tag == "Write"):
                        is_writeable = True

        return vregmap_classes.vregmap_bitfield(name, width, (top - width + 1), is_writeable)

    def get_device(self):
        return self.device

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
