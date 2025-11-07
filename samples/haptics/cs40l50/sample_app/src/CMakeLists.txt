set(DRIVER_BASE "../../../../..")

zephyr_include_directories(${DRIVER_BASE}/cs40l50/)
zephyr_library_sources(${DRIVER_BASE}/cs40l50/cs40l50.c ${DRIVER_BASE}/common/fw_img.c)
add_subdirectory(drivers)
