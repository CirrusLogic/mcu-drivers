/**
 * @file create_syscfg.c
 *
 * @brief Tool to create configuration register defaults for the CS35L41 Driver
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include "device_syscfg_regs.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint32_t updated_regs_total = 0;
static syscfg_reg_descriptor_t syscfg_reg_desc;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static void fprint_c_copyright(FILE *fp, syscfg_reg_descriptor_t *d)
{
    fprintf(fp, "\
/**\n\
 * @file ");
    fprintf(fp, "%s", d->source_filename);
    fprintf(fp, "\n\
 *\n\
 * @brief Register values to be applied after %s Driver boot().\n\
 *\n\
 * @copyright\n\
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/\n\
 *\n\
 * Licensed under the Apache License, Version 2.0 (the License); you may\n\
 * not use this file except in compliance with the License.\n\
 * You may obtain a copy of the License at\n\
 *\n\
 * www.apache.org/licenses/LICENSE-2.0\n\
 *\n\
 * Unless required by applicable law or agreed to in writing, software\n\
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT\n\
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
 * See the License for the specific language governing permissions and\n\
 * limitations under the License. \n\
 *\n\
 */\n\
", d->chip_name_uc);
    return;
}

static void fprint_h_copyright(FILE *fp, syscfg_reg_descriptor_t *d)
{
    fprintf(fp, "\
/**\n\
 * @file ");
    fprintf(fp, "%s", d->header_filename);
    fprintf(fp, "\n\
 *\n\
 * @brief Register values to be applied after %s Driver boot().\n\
 *\n\
 * @copyright\n\
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/\n\
 *\n\
 * Licensed under the Apache License, Version 2.0 (the License); you may\n\
 * not use this file except in compliance with the License.\n\
 * You may obtain a copy of the License at\n\
 *\n\
 * www.apache.org/licenses/LICENSE-2.0\n\
 *\n\
 * Unless required by applicable law or agreed to in writing, software\n\
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT\n\
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
 * See the License for the specific language governing permissions and\n\
 * limitations under the License. \n\
 *\n\
 */\n\
", d->chip_name_uc);
    return;
}

static void fprintf_include_guard_top(FILE *fp, syscfg_reg_descriptor_t *d)
{
    fprintf(fp, "\
\n\
#ifndef %s\n\
#define %s\n\
\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
", d->header_filename_uc, d->header_filename_uc);

    return;
}

static void fprintf_include_guard_bottom(FILE *fp, syscfg_reg_descriptor_t *d)
{
    fprintf(fp, "\
\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
#endif // %s\n\
\n\
", d->header_filename_uc);

    return;
}

static void fprint_includes(FILE *fp)
{
    fprintf(fp, "\
/***********************************************************************************************************************\n\
 * INCLUDES\n\
 **********************************************************************************************************************/\n\
");
    return;
}

static void fprint_literals_constants(FILE *fp)
{
    fprintf(fp, "\
/***********************************************************************************************************************\n\
 * LITERALS & CONSTANTS\n\
 **********************************************************************************************************************/\n\
");
    return;
}

static void fprint_globals(FILE *fp)
{
    fprintf(fp, "\
/***********************************************************************************************************************\n\
 * GLOBAL VARIABLES\n\
 **********************************************************************************************************************/\n\
");
    return;
}

static void fprint_typedefs(FILE *fp)
{
    fprintf(fp, "\
/***********************************************************************************************************************\n\
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS\n\
 **********************************************************************************************************************/\n\
");
    return;
}

static void export_header_file(syscfg_reg_descriptor_t *d)
{
    FILE *fp;
    fp = fopen(d->header_filename, "w");
    fprint_h_copyright(fp, d);
    fprintf_include_guard_top(fp, d);
    fprint_includes(fp);
    fprintf(fp, "#include \"stdint.h\"\n\n");
    fprint_literals_constants(fp);
    fprintf(fp, "#define %s_SYSCFG_REGS_TOTAL    (%d)\n\n", d->chip_name_uc, updated_regs_total);

    uint8_t count = 0;
    for (int i = 0; i < d->reg_list_total; i++)
    {
        if (d->reg_list[i].mask)
        {
            fprintf(fp,
                    "#define %s_%s_SYSCFG_REGS_INDEX (%d)\n",
                    d->chip_name_uc,
                    d->reg_list[i].name,
                    count++);
        }
    }
    fprintf(fp, "\n");

    add_device_header_defines(fp, d);

    fprint_typedefs(fp);

    fprintf(fp, "typedef struct\n");
    fprintf(fp, "{\n");
    fprintf(fp, "    uint32_t address;\n");
    fprintf(fp, "    uint32_t mask;\n");
    fprintf(fp, "    uint32_t value;\n");
    fprintf(fp, "} syscfg_reg_t;\n\n");

    fprint_globals(fp);
    fprintf(fp, "extern const syscfg_reg_t %s_syscfg_regs[];\n", d->chip_name_lc);
    fprintf_include_guard_bottom(fp, d);
    fclose(fp);
    return;
}

static void export_source_file(syscfg_reg_descriptor_t *d)
{
    FILE *fp;

    fp = fopen(d->source_filename, "w");
    fprint_c_copyright(fp, d);
    fprint_includes(fp);
    fprintf(fp, "#include \"");
    fprintf(fp, "%s", d->header_filename);
    fprintf(fp, "\"\n");
    fprintf(fp, "#include \"%s_spec.h\"\n\n", d->chip_name_lc);
    fprint_globals(fp);
    fprintf(fp, "const syscfg_reg_t %s_syscfg_regs[] = \n{\n", d->chip_name_lc);

    updated_regs_total = 0;

    for (int i = 0; i < d->reg_list_total; i++)
    {
        if (d->reg_list[i].mask)
        {
            fprintf(fp,
                    "    {0x%08x, 0x%08x, 0x%08x}, // %s\n",
                    d->reg_list[i].address,
                    d->reg_list[i].mask,
                    d->reg_list[i].value,
                    d->reg_list[i].name);
            updated_regs_total++;
        }
    }
    fprintf(fp, "};\n");
    fclose(fp);
    return;
}

void prepare_reg_sets(syscfg_reg_descriptor_t *d)
{
    // Prepare registers
    for (int i = 0; i < d->reg_list_total; i++)
    {
        d->cleared_regs[i] = 0x00000000;
        d->set_regs[i] = 0xFFFFFFFF;
        d->reg_list[i].mask = 0x00000000;
        d->reg_list[i].value = 0x00000000;
    }

    return;
}

static void generate_mask_set(syscfg_reg_descriptor_t *d)
{
    for (int i = 0; i < d->reg_list_total; i++)
    {
        /*
         * If a bitfield is updated in a register, at this point it will be the same value both in the 'cleared_regs'
         * (with values of all 0s) and the 'set_regs' (with values of all 1s).  Below, the mask is generated by the
         * NOT of the XOR of the 'cleared_regs' and 'set_regs'.  The XOR will set all bits to 1 which are different
         * between 'cleared_regs' and 'set_regs' - a bit set for each bit that was NOT changed.  Then the mask that
         * corresponds to updated values is obtained by the NOT of this XOR.
         */
        d->reg_list[i].mask = ~(d->cleared_regs[i] ^ d->set_regs[i]);

        if (d->reg_list[i].mask)
        {
            d->reg_list[i].value = d->cleared_regs[i] & d->reg_list[i].mask;
        }
    }

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
int main(void)
{
    configure_syscfg_reg_descriptor(&syscfg_reg_desc);

    printf("create_syscfg_regs:\n");

    printf("Creating %s_syscfg_regs[]...\n", syscfg_reg_desc.chip_name_lc);

    prepare_reg_sets(&syscfg_reg_desc);
    set_device_syscfg();
    apply_device_syscfg(syscfg_reg_desc.cleared_regs);
    apply_device_syscfg(syscfg_reg_desc.set_regs);
    generate_mask_set(&syscfg_reg_desc);

    // Write updated/configured config_reg values to .h/.h file
    printf("Writing to %s and %s...\n", syscfg_reg_desc.header_filename, syscfg_reg_desc.source_filename);

    export_source_file(&syscfg_reg_desc);
    export_header_file(&syscfg_reg_desc);

    printf("Done!\n");

    return 0;
}
