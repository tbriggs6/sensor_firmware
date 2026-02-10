# Zephyr config module (config_z). Used when building app with west.

zephyr_library()
zephyr_library_sources(config_z.c)
zephyr_library_include_directories(.)
zephyr_library_link_libraries(zephyr_interface)
