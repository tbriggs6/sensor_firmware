cmake_minimum_required(VERSION 3.20.0)

# Top-level CMakeLists for Zephyr-based builds.
# Typical usage is to build from one of the app/ subdirectories:
#   west build -b ti_cc1352r1 app/water_node
#   west build -b ti_cc1352r1 app/airborne_node
#   west build -b ti_cc1352r1 app/spinet_node
#
# This file exists mainly to document that this repository can be
# used as a Zephyr application tree.

project(sensor_firmware NONE)

