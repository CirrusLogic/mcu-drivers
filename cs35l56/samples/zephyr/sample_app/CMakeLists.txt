# SPDX-License-Identifier: Apache-2.0
set(DRIVER_BASE "../../../..")
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stream_cs35l56)

FILE(GLOB app_sources src/*.c ${DRIVER_BASE}/cs35l56/*.c ${DRIVER_BASE}/common/fw_img.c)
target_sources(app PRIVATE ${app_sources})

zephyr_library_add_dependencies(offsets_h)

zephyr_compile_definitions(CIRRUS_ZEPHYR_SAMPLE)

add_subdirectory(src)
