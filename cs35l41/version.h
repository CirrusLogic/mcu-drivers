/**
 * @file version.h
 *
 * @brief CS35L41 driver software version literals
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */

#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup VERSION_
 * @brief Defines for the release version of the driver
 *
 * @details
 * Versions for the CS35L41 driver are defined as:
 * - Major - The interface of the firmware or module has changed in a way that breaks backwards compatibility. This
 * means that the module will not work as before if the old interface is used.
 * - Minor - The interface of the firmware or module has changed, but not in a way that breaks backwards compatibility.
 * This means that the module will work as before if the old interface is used.
 * - Patch - The function has changed without changing the interface, for instance for a bug fix.
 *
 * @{
 */
#define VERSION_MAJOR   (1) ///< Release Major version
#define VERSION_MINOR   (0) ///< Release Minor version
#define VERSION_PATCH   (0) ///< Release Patch version
/** @} */


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

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // VERSION_H
