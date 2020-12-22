/**
 * @file version.h
 *
 * @brief CS47L15 driver software version literals
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
 * Versions for the CS47L15 driver are defined as:
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
