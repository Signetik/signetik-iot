# /*============================================================================*
#  *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
#  *----------------------------------------------------------------------------*
#  *                                                              Signetik, LLC *
#  *                                                           www.signetik.com *
#  *                                      SPDX-License-Identifier: BSD-4-Clause *
#  *============================================================================*/

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(signetik-client)

macro(zephyr_library_named_o name)
  # This is a macro because we need add_library() to be executed
  # within the scope of the caller.
  set(ZEPHYR_CURRENT_LIBRARY ${name})
  add_library(${name} OBJECT "")

  zephyr_append_cmake_library(${name})

  target_link_libraries(${name} PUBLIC zephyr_interface)
endmacro()

# Main app
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/LTE_task.c)
target_sources(app PRIVATE src/accel_task.c)
target_sources(app PRIVATE src/temp_humidity_task.c)
target_sources(app PRIVATE src/main_task.c)
target_sources(app PRIVATE src/cell_packet.c)

add_library (sigcoap STATIC IMPORTED libsigcoap.a)
set_target_properties(sigcoap PROPERTIES IMPORTED_LOCATION ${ZEPHYR_BASE}/../signetik-iot-pub/signetik_client_rel/lib/libsigcoap.a)

# Link library
target_link_libraries (app PRIVATE sigcoap)
