# ==========================================================================
# (c) 2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Class for exporting vregmap
# File    : vregmap_exporter.py
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

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
header_file_template_str = """/**
 * @file vregmap.h
 *
 * @brief
 * Virtual regmap interface.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef VREGMAP_H
#define VREGMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "regmap.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define VREGMAP_LENGTH_REGS         ({reg_total})
#define VREGMAP_BRIDGE_DEVICE_ID    "\\"{bridge_device_id}\\""
#define VREGMAP_BRIDGE_DEV_NAME     "{bridge_dev_name}-1"

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern const regmap_cp_config_t vregmap_cp;
extern regmap_virtual_register_t vregmap[VREGMAP_LENGTH_REGS];

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // VREGMAP_H

"""

source_file_template_str = """/**
 * @file vregmap.c
 *
 * @brief Virtual regmap operations implementation.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
{metadata_text} *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include "vregmap.h"

// Add includes for modules needed for vregmap handlers

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
{reg_defines}
/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
{reg_handler_declarations}
/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
const regmap_cp_config_t vregmap_cp =
{
    .dev_id = (uint32_t) vregmap,
    .bus_type = REGMAP_BUS_TYPE_VIRTUAL,
    .receive_max = (uint16_t) VREGMAP_LENGTH_REGS,
};

regmap_virtual_register_t vregmap[] =
{
{reg_definitions}};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
{reg_handler_definitions}"""

vregmap_reg_defines = """#define VREGMAP_{name}_ADDRESS ({address})
#define VREGMAP_{name}_DEFAULT ({default})
"""

vregmap_bitfield_defines = "#define VREGMAP_{regname}_{name}_MASK ({mask})\n"

vregmap_read_definition = "static uint32_t vregmap_read_{index}(void *self, uint32_t *val);\n"
vregmap_write_definition = "static uint32_t vregmap_write_{index}(void *self, uint32_t val);\n"

vregmap_definition_rw = """    {
        .address = VREGMAP_{name}_ADDRESS,
        .default_value = VREGMAP_{name}_DEFAULT,
        .value = VREGMAP_{name}_DEFAULT,
        .on_read = vregmap_read_{index},
        .on_write = vregmap_write_{index},
    },
"""

vregmap_definition_ro = """    {
        .address = VREGMAP_{name}_ADDRESS,
        .default_value = VREGMAP_{name}_DEFAULT,
        .value = VREGMAP_{name}_DEFAULT,
        .on_read = vregmap_read_{index},
        .on_write = NULL,
    },
"""

vregmap_read_declaration = """static uint32_t vregmap_read_{index}(void *self, uint32_t *val)
{
    regmap_virtual_register_t *s = (regmap_virtual_register_t *) self;
    uint32_t temp_value = 0;

    // TODO:  Add custom read handling code here
{read_bitfields}
    *val = s->value;

    return BSP_STATUS_OK;
}
"""

vregmap_read_bitfield = """    // Read {regname}.{name} bitfield
    temp_value &= ~VREGMAP_{regname}_{name}_MASK;
    temp_value |= (s->value & VREGMAP_{regname}_{name}_MASK);
"""

vregmap_write_declaration = """static uint32_t vregmap_write_{index}(void *self, uint32_t val)
{
    regmap_virtual_register_t *s = (regmap_virtual_register_t *) self;
    uint32_t changed_bits = s->value ^ val;

    s->value = val;

{write_bitfields}
    return BSP_STATUS_OK;
}
"""

vregmap_write_bitfield = """    // Write {regname}.{name} bitfield
    if (changed_bits & VREGMAP_{regname}_{name}_MASK)
    {
        // TODO:  Add custom write handling code here
    }
"""

# ==========================================================================
# CLASSES
# ==========================================================================
class vregmap_exporter:
    def __init__(self, output_path, device, metadata_text):
        self.output_path = output_path
        self.device = device
        self.metadata_text = metadata_text

        return

    def export(self):
        # Export the header
        f = open(self.output_path + "/vregmap.h", 'w')
        output_str = header_file_template_str.replace('{reg_total}', str(len(self.device.registers)))
        output_str = output_str.replace("{bridge_device_id}", self.device.device_id_type)
        output_str = output_str.replace("{bridge_dev_name}", self.device.device_id_type)
        f.write(output_str)
        f.close()

        # Export the source
        f = open(self.output_path + "/vregmap.c", 'w')

        defines_str = ""
        handler_declarations_str = ""
        reg_definitions_str = ""
        handler_definitions_str = ""

        for i in range(0, len(self.device.registers)):
            r = self.device.registers[i]
            temp_define_str = vregmap_reg_defines.replace("{name}", r.name).replace("{address}", "0x{:08x}".format(r.address)).replace("{default}", "0x{:08x}".format(r.default))

            temp_handler_declarations_str = vregmap_read_definition.replace("{index}", str(i))
            if (r.is_writeable):
                temp_handler_declarations_str += vregmap_write_definition.replace("{index}", str(i))

            if (r.is_writeable):
                temp_reg_definition_str = vregmap_definition_rw.replace("{name}", r.name).replace("{index}", str(i))
            else:
                temp_reg_definition_str = vregmap_definition_ro.replace("{name}", r.name).replace("{index}", str(i))

            temp_reg_read_declaration_str = vregmap_read_declaration.replace("{index}", str(i))
            temp_reg_write_declaration_str = vregmap_write_declaration.replace("{index}", str(i))

            temp_read_bitfield_str = ""
            temp_write_bitfield_str = ""
            for b in r.bitfields:
                temp_define_str += vregmap_bitfield_defines.replace("{regname}", r.name).replace("{name}", b.name).replace("{mask}", "0x{:08x}".format(b.mask))

                temp_read_bitfield_str += vregmap_read_bitfield.replace("{regname}", r.name).replace("{name}", b.name) + '\n'
                if (b.is_writeable):
                    temp_write_bitfield_str += vregmap_write_bitfield.replace("{regname}", r.name).replace("{name}", b.name) + '\n'

            temp_reg_read_declaration_str = temp_reg_read_declaration_str.replace("{read_bitfields}", temp_read_bitfield_str)
            temp_reg_write_declaration_str = temp_reg_write_declaration_str.replace("{write_bitfields}", temp_write_bitfield_str)

            defines_str += temp_define_str + '\n'
            handler_declarations_str += temp_handler_declarations_str
            reg_definitions_str += temp_reg_definition_str
            handler_definitions_str += temp_reg_read_declaration_str + '\n'
            if (r.is_writeable):
                handler_definitions_str += temp_reg_write_declaration_str + '\n'

        output_str = source_file_template_str
        temp_metadata_text = ""
        for l in self.metadata_text:
            temp_metadata_text += ' * ' + l + '\n'
        output_str = output_str.replace("{metadata_text}", temp_metadata_text)
        output_str = output_str.replace("{reg_defines}", defines_str)
        output_str = output_str.replace("{reg_handler_declarations}", handler_declarations_str)
        output_str = output_str.replace("{reg_definitions}", reg_definitions_str)
        output_str = output_str.replace("{reg_handler_definitions}", handler_definitions_str)

        output_str = output_str.replace("\n\n\n", "\n\n")

        f.write(output_str)
        f.close()

        results_str = "Exported to:\n"
        results_str += self.output_path + "/vregmap.h\n"
        results_str += self.output_path + "/vregmap.c\n"
        return results_str

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================

