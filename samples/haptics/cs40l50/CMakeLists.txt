# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cs40l50)

FILE(GLOB app_sources src/*.c ../../cs40l50/*.c ../../common/fw_img.c)
target_sources(app PRIVATE ${app_sources})

zephyr_library_add_dependencies(offsets_h)

zephyr_compile_definitions(CIRRUS_ZEPHYR_SAMPLE)

add_subdirectory(src)
