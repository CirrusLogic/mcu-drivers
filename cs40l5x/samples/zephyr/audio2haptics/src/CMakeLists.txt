set(DRIVER_BASE "../../../../..")

zephyr_include_directories(${DRIVER_BASE}/cs40l5x/)
zephyr_library_sources(${DRIVER_BASE}/cs40l5x/cs40l5x.c ${DRIVER_BASE}/common/fw_img.c)
add_subdirectory(drivers)
