set(DRIVER_BASE "../../../../..")

zephyr_include_directories(${DRIVER_BASE}/cs35l56/)
zephyr_library_sources(${DRIVER_BASE}/cs35l56/cs35l56.c ${DRIVER_BASE}/common/fw_img.c)
add_subdirectory(drivers)
