# SPDX-License-Identifier: Apache-2.0
set(DRIVER_BASE "../../../..")
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cs40l5x)

FILE(GLOB app_sources src/*.c ${DRIVER_BASE}/cs40l5x/*.c ${DRIVER_BASE}/common/fw_img.c)

# Only compile firmware data for the selected CS40L5x variant.
# Multiple firmware .c files define shared symbol names like
# cs40l5x_A2H_coeff_blocks and cs40l5x_SVC_coeff_blocks.
list(REMOVE_ITEM app_sources
	${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l51_firmware.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l52_firmware.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l53_firmware.c)

if(CONFIG_HAPTICS_CS40L51)
	list(APPEND app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l51_firmware.c)
elseif(CONFIG_HAPTICS_CS40L52)
	list(APPEND app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l52_firmware.c)
elseif(CONFIG_HAPTICS_CS40L53)
	list(APPEND app_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/cs40l53_firmware.c)
endif()

target_sources(app PRIVATE ${app_sources})

zephyr_library_add_dependencies(offsets_h)

zephyr_compile_definitions(CIRRUS_ZEPHYR_SAMPLE)

add_subdirectory(src)
