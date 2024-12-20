# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Set board root directory
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Set devicetree overlay
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf5340dk_nrf5340_cpuapp.overlay)

# Add definitions for EVE platform - these need to be before project()
add_definitions(-DLINUX_PLATFORM)
add_definitions(-DBUFFER_OPTIMIZATION)
add_definitions(-DEVE_4)
add_definitions(-DEVE_4_INTERNAL_OSC)
add_definitions(-DIPS_70)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(riverdi_eve_demo)

target_sources(app PRIVATE
    main/riverdi-demo.c
    main/platform.c
    riverdi-eve-lib/eve_layer/Gpu_Hal.c
    riverdi-eve-lib/eve_layer/CoPro_Cmds.c
    riverdi-eve-lib/eve_layer/Hal_Utils.c
    riverdi-eve-lib/app_layer/App_Common.c
)

target_include_directories(app PRIVATE
    main
    riverdi-eve-lib/riverdi_modules
    riverdi-eve-lib/eve_layer
    riverdi-eve-lib/app_layer
)
