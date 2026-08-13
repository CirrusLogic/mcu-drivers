/**
 * @file bsp_cs40l26_fw_img_v2.h
 *
 * @brief Implementation of the fw_img_v2 BSP for the cs40l26 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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

#ifndef BSP_CS40L26_FW_IMG_V2_H
#define BSP_CS40L26_FW_IMG_V2_H

#include <stdbool.h>
#include <stdint.h>

uint32_t bsp_dut_boot_fw_img_v2(bool cal_boot);
uint32_t bsp_dut_load_wavetable_fw_img_v2(void);
uint32_t bsp_dut_calibrate_fw_img_v2(void);

#endif
