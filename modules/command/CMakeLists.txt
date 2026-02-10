# Zephyr command module (command_z). Depends on config_z and messenger_z.

zephyr_library()
zephyr_library_sources(command_z.c)
zephyr_library_include_directories(.
  ${CMAKE_CURRENT_SOURCE_DIR}/../config
  ${CMAKE_CURRENT_SOURCE_DIR}/../messenger
)
zephyr_library_link_libraries(config messenger)
