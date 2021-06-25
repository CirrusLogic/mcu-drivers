# ==========================================================================
# (c) 2021 Cirrus Logic, Inc.
# --------------------------------------------------------------------------
# Project : Manage the info in sdk_version.h
# File    : sdk_version.py
# --------------------------------------------------------------------------
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
# --------------------------------------------------------------------------
#
# Environment Requirements: None
#
# ==========================================================================

# ==========================================================================
# IMPORTS
# ==========================================================================
import os
import sys
repo_path = os.path.dirname(os.path.abspath(__file__)) + '/../..'
import argparse
import datetime

# ==========================================================================
# VERSION
# ==========================================================================

# ==========================================================================
# CONSTANTS/GLOBALS
# ==========================================================================
supported_commands = ['get', 'set']

sdk_version_h_template = """/**
 * @file sdk_version.h
 *
 * @brief Alt-OS SDK version literals
 *
 * @copyright
 * Copyright (c) Cirrus Logic {year} All Rights Reserved, http://www.cirrus.com/
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
 * - Update - The function has changed without changing the interface, for instance for a bug fix.
 *
 * @{
 */
#define SDK_VERSION_MAJOR       ({major_version}) ///< Release Major version
#define SDK_VERSION_MINOR       ({minor_version}) ///< Release Minor version
#define SDK_VERSION_UPDATE      ({update_version}) ///< Release Update version
#define SDK_GIT_SHA             ("{git_sha}") ///< Release Git SHA
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
"""

# ==========================================================================
# CLASSES
# ==========================================================================
class sdk_version:
    def __init__(self, major: str, minor: str, update: str, sha: str):
        self.m = major
        self.n = minor
        self.u = update
        self.sha = sha

        return

    @classmethod
    def getVersions(cls, fn):

        f = open(fn, 'r')
        lines = f.readlines()
        f.close()

        versions = ['0', '0', '0', '0']

        for l in lines:
            if ('SDK_VERSION_MAJOR' in l):
                l = l.split('(')[1]
                l = l.split(')')
                versions[0] = l[0]
            if ('SDK_VERSION_MINOR' in l):
                l = l.split('(')[1]
                l = l.split(')')
                versions[1] = l[0]
            if ('SDK_VERSION_UPDATE' in l):
                l = l.split('(')[1]
                l = l.split(')')
                versions[2] = l[0]
            if ('SDK_GIT_SHA' in l):
                l = l.split('("')[1]
                l = l.split('")')
                versions[3] = l[0]

        return cls(major = versions[0], minor = versions[1], update = versions[2], sha = versions[3])

    @classmethod
    def setVersions(cls, fn, m, n, u, sha):

        if ((m is None) or (n is None) or (u is None) or (sha is None)):
            tempcls = cls.getVersions(fn)
            if (m is None):
                m = tempcls.m
            if (n is None):
                n = tempcls.n
            if (u is None):
                u = tempcls.u
            if (sha is None):
                sha = tempcls.sha

        f = open(fn, 'w')
        lines = sdk_version_h_template
        lines = lines.replace('{major_version}', m)
        lines = lines.replace('{minor_version}', n)
        lines = lines.replace('{update_version}', u)
        lines = lines.replace('{git_sha}', sha)
        lines = lines.replace('{year}', str(datetime.date.today().year))
        f.writelines(lines)
        f.close()

        return cls(major = m, minor = n, update = u, sha = sha)

    def __str__(self):
        return (self.m + '.' + self.n + '.' + self.u + ' - ' + self.sha)

# ==========================================================================
# HELPER FUNCTIONS
# ==========================================================================
def print_sdk_version(filename):
    return str(sdk_version.getVersions(filename))

def get_args(args):
    """Parse arguments"""
    parser = argparse.ArgumentParser(description='Parse command line arguments')
    parser.add_argument('-c', '--command', dest='command', type=str, choices=supported_commands, required=True,
                        help='The command you wish to execute.')
    parser.add_argument('-f', '--file', dest='sdk_version_h_filename', type=str, default='./sdk_verion.h',
                        help='Path and filename of the sdk_version.h to manage.')
    parser.add_argument('-m', '--major', dest='major_version', type=str, default=None, help='The major SDK version.')
    parser.add_argument('-n', '--minor', dest='minor_version', type=str, default=None, help='The minor SDK version.')
    parser.add_argument('-u', '--update', dest='update_version', type=str, default=None, help='The update SDK version.')
    parser.add_argument('-s', '--sha', dest='git_sha', type=str, default=None, help='The Git SHA for the SDK version.')
    parser.add_argument('-o', '--oneline', dest='oneline', action="store_true", help='Only print one line version')

    return parser.parse_args(args[1:])

def validate_args(args):
    ret = True
    # Check that sdk_version.h exists for 'get'command
    if (args.command == 'get') and (not os.path.exists(args.sdk_version_h_filename)):
        print("Invalid sdk_version.h filename: " + args.sdk_version_h_filename)
        ret = False

    # Check that major/minor/update and SHA are specified for 'set' command
    if (args.command == 'set'):
        if ((args.major_version is None) and (args.minor_version is None) and (args.update_version is None) and (args.git_sha is None)):
             print("No version parameters are specified.")
             ret = False

        # oneline is only valid for 'get'
        args.oneline = False

    return ret

def print_start():
    print("")
    print("sdk_version")
    print("Manage the info in sdk_version.h")
    print("SDK Version " + print_sdk_version(repo_path + '/sdk_version.h'))
    print("")

    return

def print_args(args):
    print("")
    print("Command: " + args.command)
    print("sdk_version.h filename: " + args.sdk_version_h_filename)
    if (args.command == 'set'):
        print("Major version: " + str(args.major_version))
        print("Minor version: " + str(args.minor_version))
        print("Update version: " + str(args.update_version))
        print("Git SHA: " + str(args.git_sha))
    if (args.oneline):
            print("Oneline: true")

    print("")

    return

def print_results(results_string):
    print(results_string)

    return

def print_end():
    print("Exit.")

    return

def error_exit(error_message):
    print('ERROR: ' + error_message)
    exit(1)

# ==========================================================================
# MAIN PROGRAM
# ==========================================================================
def main(argv):
    args = get_args(argv)

    if (not (validate_args(args))):
        print_start()
        print_args(args)
        error_exit("Invalid Arguments")

    if (not args.oneline):
        print_start()
        print_args(args)

    results_str = '\n'

    if (args.command == 'get'):
        sdk_version_str = str(sdk_version.getVersions(args.sdk_version_h_filename))
        results_str += sdk_version_str
    else:
        v = sdk_version.setVersions(args.sdk_version_h_filename,
                                    args.major_version,
                                    args.minor_version,
                                    args.update_version,
                                    args.git_sha)
        results_str += 'File written'

    if (not args.oneline):
        print_results(results_str)
        print_end()
    else:
        print(sdk_version_str.split(' - ')[0])

    return


if __name__ == "__main__":
    main(sys.argv)
