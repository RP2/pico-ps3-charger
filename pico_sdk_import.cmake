# Pico SDK Import
#
# This file is used by CMake to find and import the Pico SDK.
# It checks for the SDK in several locations and downloads it if needed.

if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (DEFINED PICO_SDK_PATH)
    message(STATUS "PICO_SDK_PATH = ${PICO_SDK_PATH}")
else()
    # Try to find the SDK in common locations
    set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")
    if (NOT EXISTS "${PICO_SDK_PATH}/pico_sdk_init.cmake")
        set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../pico-sdk")
        if (NOT EXISTS "${PICO_SDK_PATH}/pico_sdk_init.cmake")
            # Auto-clone the SDK
            message(STATUS "Pico SDK not found, cloning...")
            execute_process(
                COMMAND git clone --depth 1 https://github.com/raspberrypi/pico-sdk.git "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk"
                RESULT_VARIABLE PICO_SDK_CLONE_RESULT
            )
            if (NOT PICO_SDK_CLONE_RESULT EQUAL 0)
                message(FATAL_ERROR "Failed to clone Pico SDK. Please set PICO_SDK_PATH manually.")
            endif()
            # Also clone submodules
            execute_process(
                COMMAND git submodule update --init
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk"
            )
            set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")
        endif()
    endif()
endif()

# Import the SDK
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)