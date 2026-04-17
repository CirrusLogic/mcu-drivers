#include "rth_types.h"
#include <stddef.h>
#include <stdlib.h>
const rth_pwle_section_t pwle_1_section1 =
{
    .duration = 0,
    .level = 0,
    .freq = 800,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_1_section2 =
{
    .duration = 4,
    .level = 0,
    .freq = 800,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle1_sections[2] =
{
    &pwle_1_section1,
    &pwle_1_section2
};

const rth_pwle_t pwle_1 =
{
    .sections = pwle1_sections,
    .num_sections = 2,
    .length_us = 1000,
    .name = "null_1ms_wave",
    .msft_id = 0
};
const rth_pwle_section_t pwle_2_section1 =
{
    .duration = 0,
    .level = 165,
    .freq = 640,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_2_section2 =
{
    .duration = 25,
    .level = 0,
    .freq = 640,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle2_sections[2] =
{
    &pwle_2_section1,
    &pwle_2_section2
};

const rth_pwle_t pwle_2 =
{
    .sections = pwle2_sections,
    .num_sections = 2,
    .length_us = 26250,
    .name = "Hover",
    .msft_id = 0x1008
};
const rth_pwle_section_t pwle_3_section1 =
{
    .duration = 0,
    .level = 0,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_3_section2 =
{
    .duration = 40,
    .level = 401,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_3_section3 =
{
    .duration = 0,
    .level = 0,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_3_section4 =
{
    .duration = 158,
    .level = 0,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_3_section5 =
{
    .duration = 0,
    .level = 401,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_3_section6 =
{
    .duration = 40,
    .level = 0,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle3_sections[6] =
{
    &pwle_3_section1,
    &pwle_3_section2,
    &pwle_3_section3,
    &pwle_3_section4,
    &pwle_3_section5,
    &pwle_3_section6
};

const rth_pwle_t pwle_3 =
{
    .sections = pwle3_sections,
    .num_sections = 6,
    .length_us = 79500,
    .name = "Collide",
    .msft_id = 0x1012
};
const rth_pwle_section_t pwle_4_section1 =
{
    .duration = 0,
    .level = 335,
    .freq = 1556,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_4_section2 =
{
    .duration = 5,
    .level = 335,
    .freq = 1556,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle4_sections[2] =
{
    &pwle_4_section1,
    &pwle_4_section2
};

const rth_pwle_t pwle_4 =
{
    .sections = pwle4_sections,
    .num_sections = 2,
    .length_us = 21250,
    .name = "Align",
    .msft_id = 0x1013
};
const rth_pwle_section_t pwle_5_section1 =
{
    .duration = 0,
    .level = 335,
    .freq = 544,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_5_section2 =
{
    .duration = 44,
    .level = 335,
    .freq = 544,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle5_sections[2] =
{
    &pwle_5_section1,
    &pwle_5_section2
};

const rth_pwle_t pwle_5 =
{
    .sections = pwle5_sections,
    .num_sections = 2,
    .length_us = 31000,
    .name = "Step",
    .msft_id = 0x1014
};
const rth_pwle_section_t pwle_6_section1 =
{
    .duration = 0,
    .level = 0,
    .freq = 404,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_6_section2 =
{
    .duration = 78,
    .level = 99,
    .freq = 508,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t pwle_6_section3 =
{
    .duration = 107,
    .level = 368,
    .freq = 748,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t *pwle6_sections[3] =
{
    &pwle_6_section1,
    &pwle_6_section2,
    &pwle_6_section3
};

const rth_pwle_t pwle_6 =
{
    .sections = pwle6_sections,
    .num_sections = 3,
    .length_us = 66250,
    .name = "Grow",
    .msft_id = 0x1015
};
const rth_pwle_section_t pwle_7_section1 =
{
    .duration = 0,
    .level = 401,
    .freq = 536,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_7_section2 =
{
    .duration = 45,
    .level = 0,
    .freq = 536,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle7_sections[2] =
{
    &pwle_7_section1,
    &pwle_7_section2
};

const rth_pwle_t pwle_7 =
{
    .sections = pwle7_sections,
    .num_sections = 2,
    .length_us = 31250,
    .name = "Press",
    .msft_id = 0x1006
};
const rth_pwle_section_t pwle_8_section1 =
{
    .duration = 0,
    .level = 0,
    .freq = 1232,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_8_section2 =
{
    .duration = 19,
    .level = 401,
    .freq = 1232,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle8_sections[2] =
{
    &pwle_8_section1,
    &pwle_8_section2
};

const rth_pwle_t pwle_8 =
{
    .sections = pwle8_sections,
    .num_sections = 2,
    .length_us = 24750,
    .name = "Release",
    .msft_id = 0x1007
};
const rth_pwle_section_t pwle_9_section1 =
{
    .duration = 0,
    .level = 236,
    .freq = 800,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_9_section2 =
{
    .duration = 27,
    .level = 236,
    .freq = 880,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t pwle_9_section3 =
{
    .duration = 0,
    .level = 0,
    .freq = 880,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t pwle_9_section4 =
{
    .duration = 218,
    .level = 0,
    .freq = 880,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t pwle_9_section5 =
{
    .duration = 258,
    .level = 735,
    .freq = 2384,
    .chirp = true,
    .half_cycles = false
};

const rth_pwle_section_t *pwle9_sections[5] =
{
    &pwle_9_section1,
    &pwle_9_section2,
    &pwle_9_section3,
    &pwle_9_section4,
    &pwle_9_section5
};

const rth_pwle_t pwle_9 =
{
    .sections = pwle9_sections,
    .num_sections = 5,
    .length_us = 145750,
    .name = "Success",
    .msft_id = 0x1009
};
const rth_pwle_section_t pwle_10_section1 =
{
    .duration = 0,
    .level = 401,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_10_section2 =
{
    .duration = 198,
    .level = 401,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_10_section3 =
{
    .duration = 0,
    .level = 0,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_10_section4 =
{
    .duration = 132,
    .level = 0,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_10_section5 =
{
    .duration = 0,
    .level = 401,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t pwle_10_section6 =
{
    .duration = 198,
    .level = 401,
    .freq = 1212,
    .chirp = false,
    .half_cycles = false
};

const rth_pwle_section_t *pwle10_sections[6] =
{
    &pwle_10_section1,
    &pwle_10_section2,
    &pwle_10_section3,
    &pwle_10_section4,
    &pwle_10_section5,
    &pwle_10_section6
};

const rth_pwle_t pwle_10 =
{
    .sections = pwle10_sections,
    .num_sections = 6,
    .length_us = 152000,
    .name = "Error",
    .msft_id = 0x100A
};
uint32_t const pwleCount = 10;
const rth_pwle_t *pwleList[10] =
{
    &pwle_1,
    &pwle_2,
    &pwle_3,
    &pwle_4,
    &pwle_5,
    &pwle_6,
    &pwle_7,
    &pwle_8,
    &pwle_9,
    &pwle_10,
};
