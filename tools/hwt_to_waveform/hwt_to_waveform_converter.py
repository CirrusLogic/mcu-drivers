#==========================================================================
# (c) 2022 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Convert from HWT Files to C Header/Source
# File    : hwt_to_waveform_converter.py
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
import json
import sys
from hwt_to_waveform_templates import pwle_section, pwle_section_list, pcm_header, pcm_data

#==========================================================================
# MAIN PROGRAM
#==========================================================================
print("waveform_converter\n")
dev = sys.argv[1]
f = open(sys.argv[2])
fwave_c = open("waveforms.c", "w")
fwave_h = open("waveforms.h", "w")
data = json.load(f)
file_output_str = '#include "rth_types.h"\n#include <stddef.h>\n#include <stdlib.h>\n'
file_h_output_str = '#include "rth_types.h"\n#include <stddef.h>\n#include <stdlib.h>\n\n'
pwle_num = 1
pcm_num = 1
for entry in data["Entries"]:
    if entry["Header"]["Type"] == 'Type12':
        sec_num = 1
        pwle_str = ''
        for i in entry["Body"]["Sections"]:
            psec = pwle_section(pwle_num, sec_num, i["Time"]["Value"], i["Level"]["Value"], i["Frequency"]["Value"], str(i["ChirpMode"]["Value"]), str(i["Time"]["HalfCycles"]))
            pwle_str = pwle_str + str(psec) + '\n'
            sec_num += 1
        plist = pwle_section_list(pwle_num, entry["Body"]["NumberOfSections"]["Value"])
        pwle_str = pwle_str + str(plist)
        file_output_str += pwle_str
        file_h_output_str += 'extern rth_pwle_section_t *pwle' + str(pwle_num) + '[];\n'
        file_h_output_str += 'extern uint32_t pwle_' + str(pwle_num) + '_size;\n\n'
        pwle_num += 1
    else:
        pcm_str = ''
        pcm_str = pcm_str + str(pcm_header(entry["Body"]["F0"]["Value"], entry["Body"]["ScaledReDc"]["Value"], pcm_num, dev))
        pcm_str = pcm_str + str(pcm_data(entry["Body"]["Samples"], pcm_num)) + '\n'
        file_output_str += pcm_str
        file_h_output_str += 'extern uint16_t pcm_' + str(pcm_num) + '_f0;\n'
        file_h_output_str += 'extern uint16_t pcm_' + str(pcm_num) + '_redc;\n'
        file_h_output_str += 'extern uint8_t pcm_' + str(pcm_num)+ '_data[];\n'
        file_h_output_str += 'extern uint32_t pcm_' + str(pcm_num) + '_data_size;\n\n'
        pcm_num += 1
fwave_c.write(file_output_str)
fwave_h.write(file_h_output_str)

