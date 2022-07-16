import json

pwle_section_template_str = """static rth_pwle_section_t pwle_{pwle_num}_section{num} =
{
    .duration = {duration},
    .level = {level},
    .freq = {frequency},
    .chirp = {chirp},
    .half_cycles = {half_cycles}
};
"""

pwle_list_section_template_str = """&pwle_{pwle_num}_section{section_num}"""

pwle_section_list_template_str = """rth_pwle_section_t *pwle{pwle_num}[{num_sections}] =
{
    {pwle_sections}
};

uint32_t pwle_{pwle_num}_size = {num_sections};
"""
pcm_f0_redc_template = """
uint16_t pcm_{pcm_num}_f0 = {f0};
uint16_t pcm_{pcm_num}_redc = {redc};
"""

pcm_data_template = """
uint8_t pcm_{pcm_num}_data[{num_samples}] =
{
    {data_samples}
};

uint32_t pcm_{pcm_num}_data_size = {num_samples};
"""

#==========================================================================
# CLASSES
#==========================================================================

class duration:
    def __init__(self, duration, halfcycles):
        self.output_str = ''
        self.duration = duration
        self.halfcycles = halfcycles

    def __str__(self):
        if eval(self.halfcycles):
            output_str = str(int(float(self.duration)))
            return output_str
        else:
            output_str = str(int(float(self.duration) * 4))
            return output_str

class level:
    def __init__(self, level):
        self.output_str = ''
        self.level = level

    def __str__(self):
        n = float(self.level)
        if n > 0:
            n = n / .9995
            n = 2047 * n
            output_str = str(round(n))
            return output_str
        elif n == 0:
            output_str = str(int(n))
            return output_str
        else:
            n = 2047 * abs(n)
            n = 2047 + n
            output_str = str(round(n))
            return output_str

class frequency:
    def __init__(self, frequency):
        self.output_str = ''
        self.freq = frequency
    def __str__(self):
        output_str = str((int(float(self.freq))) * 4)
        return output_str

class f0:
    def __init__(self, f0):
        self.output_str = ''
        self.f0 = f0
    def __str__(self):
        if int(self.f0) > 0:
            output_str = str(int((self.f0 - 50) * 8))
            return output_str
        else:
            output_str = '0'
            return output_str

class redc:
    def __init__(self, redc):
        self.output_str = ''
        self.redc = redc
    def __str__(self):
        output_str = str(round((float(self.redc) / 5.857) * 128))
        return output_str

class pcm_sample:
    def __init__(self, sample):
        self.output_str = ''
        self.sample = sample
    def __str__(self):
        n = float(self.sample)
        if n > 0:
            n = 127 * n
            output_str = str(abs(round(n)))
            return output_str
        elif n == 0:
            output_str = '0'
            return output_str
        else:
            n = 127 * n
            n = 256 + round(n)
            output_str = str(n)
            return output_str

class pwle_section:
    def __init__(self, pwle_num, num, duration, level, freq, chirp, half_cycles):
        self.template_str = pwle_section_template_str
        self.output_str = ''
        self.terms = dict()
        self.terms['pwle_num'] = str(pwle_num)
        self.terms['num'] = str(num)
        self.terms['duration'] = str(duration)
        self.terms['level'] = str(level)
        self.terms['frequency'] = str(freq)
        self.terms['chirp'] = chirp
        self.terms['halfcycles'] = half_cycles
        return
    def __str__(self):
        output_str = self.template_str
        output_str = output_str.replace('{pwle_num}', self.terms['pwle_num'])
        output_str = output_str.replace('{num}', self.terms['num'])
        duration_str = str(duration(self.terms['duration'], self.terms['halfcycles']))
        output_str = output_str.replace('{duration}', duration_str)
        output_str = output_str.replace('{level}', str(level(self.terms['level'])))
        output_str = output_str.replace('{frequency}', str(frequency(self.terms['frequency'])))
        if eval(self.terms['chirp']):
            output_str = output_str.replace('{chirp}', 'true')
        else:
            output_str = output_str.replace('{chirp}', 'false')
        if eval(self.terms['halfcycles']):
            output_str = output_str.replace('{half_cycles}', 'true')
        else:
            output_str = output_str.replace('{half_cycles}', 'false')
        return output_str

class pwle_section_list:
    def __init__(self, pwle_num, num_sections):
        self.template_str = pwle_section_list_template_str
        self.output_str = ''
        self.pwle_num = str(pwle_num)
        self.num_sections = num_sections

    def __str__(self):
        output_str = self.template_str
        output_str = output_str.replace('{pwle_num}', self.pwle_num)
        output_str = output_str.replace('{num_sections}', str(self.num_sections))
        list_str = ''
        for i in range(1, int(self.num_sections)+1):
            line = ''
            line = line + pwle_list_section_template_str.replace('{pwle_num}', self.pwle_num)
            line = line.replace('{section_num}', str(i))
            line = line.replace('{num_sections}', str(self.num_sections))
            list_str = list_str + line
            if i != int(self.num_sections):
                list_str = list_str + ',' + '\n' + '    '
        output_str = output_str.replace('{pwle_sections}', list_str)
        return output_str

class pcm_header:
    def __init__(self, f0, redc, pcm_num):
        self.template_str = pcm_f0_redc_template
        self.output_str = ''
        self.f0 = f0
        self.redc = redc
        self.pcm_num = pcm_num
    def __str__(self):
        output_str = self.template_str
        output_str = output_str.replace('{pcm_num}', str(self.pcm_num))
        output_str = output_str.replace('{f0}', str(f0(self.f0)))
        output_str = output_str.replace('{redc}', str(redc(self.redc)))
        return output_str

class pcm_data:
    def __init__(self, pcm_sample_list, pcm_num):
        self.template_str = pcm_data_template
        self.output_str = ''
        self.pcm_sample_list = pcm_sample_list
        self.pcm_num = pcm_num
    def __str__(self):
        output_str = self.template_str
        output_str = output_str.replace('{pcm_num}', str(self.pcm_num))
        output_str = output_str.replace('{num_samples}', str(len(self.pcm_sample_list)))
        pcm_list_str = ''
        index = 1
        for i in self.pcm_sample_list:
            pcm_list_str = pcm_list_str + str(pcm_sample(i))
            if index != (len(self.pcm_sample_list)):
                pcm_list_str = pcm_list_str + ',\n    '
            index += 1
        output_str = output_str.replace('{data_samples}', pcm_list_str)
        return output_str
