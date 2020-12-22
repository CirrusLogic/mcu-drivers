/**
 * @file cs47l15_syscfg_regs.c
 *
 * @brief Register values to be applied after CS47L15 Driver boot().
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
#include "cs47l15_syscfg_regs.h"
#include "cs47l15_spec.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
const syscfg_reg_t cs47l15_syscfg_regs[] =
{
    {0x00000100, 0xFFFFFFFF, 0x00000041}, 
    {0x00000171, 0xFFFFFFFF, 0x00000002}, 
    {0x00000172, 0xFFFFFFFF, 0x000003E8}, 
    {0x00000173, 0xFFFFFFFF, 0x00000000}, 
    {0x00000174, 0xFFFFFFFF, 0x00000000}, 
    {0x00000175, 0xFFFFFFFF, 0x00000200}, 
    {0x00000176, 0xFFFFFFFF, 0x00000001}, 
    {0x00000179, 0xFFFFFFFF, 0x00000008}, 
    {0x00000172, 0xFFFFFFFF, 0x000083E8}, 
    {0x00000101, 0xFFFFFFFF, 0x00000404}, 
    {0x00000102, 0xFFFFFFFF, 0x00000003}, 
    {0x00000122, 0xFFFFFFFF, 0x000024DD}, 
    {0x00000218, 0xFFFFFFFF, 0x000081A4}, 
    {0x00000310, 0xFFFFFFFF, 0x00000C80}, 
};

