/**
 * @file cs40l25_sym.h
 *
 * @brief Master table of known firmware symbols for the CS40L25 Driver module
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

#ifndef CS40L25_SYM_H
#define CS40L25_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L25_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// Firmware - GENERAL
#define CS40L25_SYM_GENERAL_HALO_STATE                                  (0x1)
#define CS40L25_SYM_GENERAL_HALO_HEARTBEAT                              (0x2)
#define CS40L25_SYM_GENERAL_EVENTCONTROL                                (0x3)
#define CS40L25_SYM_GENERAL_GPIO1EVENT                                  (0x4)
#define CS40L25_SYM_GENERAL_GPIO2EVENT                                  (0x5)
#define CS40L25_SYM_GENERAL_GPIO3EVENT                                  (0x6)
#define CS40L25_SYM_GENERAL_GPIO4EVENT                                  (0x7)
#define CS40L25_SYM_GENERAL_GPIOPLAYBACKEVENT                           (0x8)
#define CS40L25_SYM_GENERAL_TRIGGERPLAYBACKEVENT                        (0x9)
#define CS40L25_SYM_GENERAL_RXREADYEVENT                                (0xA)
#define CS40L25_SYM_GENERAL_HARDWAREEVENT                               (0xB)
#define CS40L25_SYM_GENERAL_INDEXBUTTONPRESS                            (0xC)
#define CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE                          (0xD)
#define CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT                           (0xE)
#define CS40L25_SYM_GENERAL_GAIN_CONTROL                                (0xF)
#define CS40L25_SYM_GENERAL_GPIO_ENABLE                                 (0x10)
#define CS40L25_SYM_GENERAL_POWERSTATE                                  (0x11)
#define CS40L25_SYM_GENERAL_POWERONSEQUENCE                             (0x12)
#define CS40L25_SYM_GENERAL_F0_STORED                                   (0x13)
#define CS40L25_SYM_GENERAL_Q_STORED                                    (0x14)
#define CS40L25_SYM_GENERAL_IRQMASKSEQUENCE                             (0x15)
#define CS40L25_SYM_GENERAL_IRQMASKSEQUENCE_VALID                       (0x16)
#define CS40L25_SYM_GENERAL_REDC_STORED                                 (0x17)
// CAL Firmware - GENERAL
#define CS40L25_SYM_GENERAL_SHUTDOWNREQUEST                             (0x18)
// VIBEGEN
#define CS40L25_SYM_VIBEGEN_TIMEOUT_MS                                  (0x19)
// CLAB
#define CS40L25_SYM_CLAB_CLAB_ENABLED                                   (0x1A)
#define CS40L25_SYM_CLAB_PEAK_AMPLITUDE_CONTROL                         (0x1B)
// CAL Firmware - F0_TRACKING
#define CS40L25_SYM_F0_TRACKING_F0                                      (0x1C)
#define CS40L25_SYM_F0_TRACKING_CLOSED_LOOP                             (0x1D)
#define CS40L25_SYM_F0_TRACKING_REDC                                    (0x1E)
#define CS40L25_SYM_F0_TRACKING_F0_TRACKING_ENABLE                      (0x1F)
#define CS40L25_SYM_F0_TRACKING_MAXBACKEMF                              (0x20)
// CAL Firmware - Q_ESTIMATION
#define CS40L25_SYM_Q_ESTIMATION_Q_EST                                  (0x21)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L25_SYM_H
