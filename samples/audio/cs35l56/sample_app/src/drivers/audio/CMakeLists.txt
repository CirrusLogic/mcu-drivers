# SPDX-License-Identifier: Apache-2.0
set(DRIVER_BASE "../../../../../../..")

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/audio/codec.h)
zephyr_library_add_dependencies(offsets_h)
zephyr_library_sources_ifdef(CONFIG_AUDIO_CS35L56 ${DRIVER_BASE}/cs35l56/cs35l56.c ../../cs35l56_bsp.c ${DRIVER_BASE}/common/fw_img.c)
zephyr_include_directories(${DRIVER_BASE}/cs35l56/ ${DRIVER_BASE}/common ${DRIVER_BASE} ../../)
