# SPDX-License-Identifier: Apache-2.0
set(DRIVER_BASE "../../../../../../..")

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/haptics.h)
zephyr_library_add_dependencies(offsets_h)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L5X_ALT_OS ${DRIVER_BASE}/cs40l5x/cs40l5x.c ../../cs40l5x_bsp.c ${DRIVER_BASE}/common/fw_img.c)
zephyr_include_directories(${DRIVER_BASE}/cs40l5x/ ${DRIVER_BASE}/common ${DRIVER_BASE} ../../)
