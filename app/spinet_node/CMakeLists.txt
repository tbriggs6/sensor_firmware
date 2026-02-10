cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spinet_node)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/net_echo
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/config
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/messenger
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/command
)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/net_echo ${CMAKE_BINARY_DIR}/net_echo)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/config ${CMAKE_BINARY_DIR}/config)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/messenger ${CMAKE_BINARY_DIR}/messenger)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/command ${CMAKE_BINARY_DIR}/command)
target_link_libraries(app PRIVATE net_echo command)

