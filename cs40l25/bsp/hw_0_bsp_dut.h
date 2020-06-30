/**
 * @file hw_0_bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the system_test_hw_0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */

#ifndef HW_0_BSP_DUT_H
#define HW_0_BSP_DUT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "bsp_driver_if.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x80)

#define BSP_HAPTIC_CONTROL_SET_BHM_BUZZ_TRIGGER             (3)
#define BSP_HAPTIC_CONTROL_SET_TRIGGER_INDEX                (4)
#define BSP_HAPTIC_CONTROL_SET_TRIGGER_MS                   (5)
#define BSP_HAPTIC_CONTROL_SET_TIMEOUT_MS                   (6)
#define BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE                  (7)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT          (8)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT          (9)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT          (10)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT          (11)
#define BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED                 (12)
#define BSP_HAPTIC_CONTROL_SET_GPI_GAIN_CONTROL             (13)
#define BSP_HAPTIC_CONTROL_SET_CTRL_PORT_GAIN_CONTROL       (14)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS     (15)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_INDEX_BUTTON_PRESS     (16)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_INDEX_BUTTON_PRESS     (17)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_INDEX_BUTTON_PRESS     (18)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE   (19)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_INDEX_BUTTON_RELEASE   (20)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_INDEX_BUTTON_RELEASE   (21)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_INDEX_BUTTON_RELEASE   (22)

#define BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT               (24)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void);
uint32_t bsp_dut_reset(void);
uint32_t bsp_dut_boot(bool cal_boot);
uint32_t bsp_dut_calibrate(void);
uint32_t bsp_dut_power_up(void);
uint32_t bsp_dut_power_down(void);
uint32_t bsp_dut_hibernate(void);
uint32_t bsp_dut_wake(void);
uint32_t bsp_dut_start_i2s(void);
uint32_t bsp_dut_stop_i2s(void);
uint32_t bsp_dut_control(uint32_t id, void *arg);
uint32_t bsp_haptic_dynamic_calibrate(void);
uint32_t bsp_dut_process(void);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_DUT_H
