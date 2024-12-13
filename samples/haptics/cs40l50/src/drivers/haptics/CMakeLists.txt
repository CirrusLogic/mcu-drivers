# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/haptics.h)
zephyr_library_add_dependencies(offsets_h)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L50 ../../../../../../cs40l50/cs40l50.c ../../cs40l50_bsp.c ../../../../../../common/fw_img.c)
zephyr_include_directories(../../../../../../cs40l50/ ../../../../../../common ../../../../../../ ../../)
