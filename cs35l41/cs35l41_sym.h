/**
 * @file cs35l41_sym.h
 *
 * @brief Master table of known firmware symbols for the CS35L41 Driver module
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

#ifndef CS35L41_SYM_H
#define CS35L41_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// Firmware - GENERAL
#define CS35L41_SYM_GENERAL_HALO_STATE                                  (0x1)
#define CS35L41_SYM_GENERAL_HALO_HEARTBEAT                              (0x2)
// CSPL
#define CS35L41_SYM_CSPL_CSPL_STATE                                     (0x3)
#define CS35L41_SYM_CSPL_CSPL_TEMPERATURE                               (0x4)
#define CS35L41_SYM_CSPL_CAL_R                                          (0x5)
#define CS35L41_SYM_CSPL_CAL_AMBIENT                                    (0x6)
#define CS35L41_SYM_CSPL_CAL_STATUS                                     (0x7)
#define CS35L41_SYM_CSPL_CAL_CHECKSUM                                   (0x8)
#define CS35L41_SYM_CSPL_CAL_R_SELECTED                                 (0x9)
#define CS35L41_SYM_CSPL_CAL_SET_STATUS                                 (0xA)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_SYM_H
