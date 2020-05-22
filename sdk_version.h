/**
 * @file sdk_version.h
 *
 * @brief Alt-OS SDK version literals
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

#ifndef SDK_VERSION_H
#define SDK_VERSION_H

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
 * @defgroup SDK_VERSION_
 * @brief Defines for the release version of the SDK
 *
 * @details
 * Versions for the SDK are defined as:
 * - Major - The interface of the firmware or module has changed in a way that breaks backwards compatibility. This
 * means that the module will not work as before if the old interface is used.
 * - Minor - The interface of the firmware or module has changed, but not in a way that breaks backwards compatibility.
 * This means that the module will work as before if the old interface is used.
 * - Patch - The function has changed without changing the interface, for instance for a bug fix.
 *
 * @{
 */
#define SDK_VERSION_MAJOR   (1) ///< Release Major version
#define SDK_VERSION_MINOR   (1) ///< Release Minor version
#define SDK_VERSION_PATCH   (0) ///< Release Patch version
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

#endif // SDK_VERSION_H
