# Zephyr net echo module (UDP echo for 6LoWPAN validation).
# Used by app/water_node, app/airborne_node, app/spinet_node.

zephyr_library()
zephyr_library_sources(echo_z.c)
zephyr_library_include_directories(.)
