# PS3 Pico Charger - CMake Build Configuration

cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK
include(pico_sdk_import.cmake)

project(ps3-pico-charger C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK first (provides pico_generate_pio_header)
pico_sdk_init()

# Fetch Pico-PIO-USB library (must come after pico_sdk_init)
include(FetchContent)
FetchContent_Declare(
    pico_pio_usb
    GIT_REPOSITORY https://github.com/sekigon-gonnoc/Pico-PIO-USB.git
    GIT_TAG        0.7.2
)
FetchContent_MakeAvailable(pico_pio_usb)

# Tell the Pico SDK's TinyUSB integration where to find Pico-PIO-USB
# This enables the tinyusb_pico_pio_usb target which provides hcd_pio_usb.c
set(PICO_PIO_USB_PATH ${pico_pio_usb_SOURCE_DIR})

# Re-check for PIO-USB support now that the path is set
if (COMMAND pico_generate_pio_header AND NOT TARGET tinyusb_pico_pio_usb)
    if (EXISTS ${PICO_PIO_USB_PATH}/src/pio_usb.c)
        add_library(tinyusb_pico_pio_usb INTERFACE)
        target_sources(tinyusb_host_base INTERFACE
                ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
                )
        target_sources(tinyusb_pico_pio_usb INTERFACE
                ${PICO_PIO_USB_PATH}/src/pio_usb.c
                ${PICO_PIO_USB_PATH}/src/pio_usb_host.c
                ${PICO_PIO_USB_PATH}/src/pio_usb_device.c
                ${PICO_PIO_USB_PATH}/src/usb_crc.c
                )
        target_include_directories(tinyusb_pico_pio_usb INTERFACE
                ${PICO_PIO_USB_PATH}/src
                )
        target_link_libraries(tinyusb_pico_pio_usb INTERFACE
                hardware_dma
                hardware_pio
                pico_multicore
                )
        target_compile_definitions(tinyusb_pico_pio_usb INTERFACE
                PIO_USB_USE_TINYUSB
                )
        pico_generate_pio_header(tinyusb_pico_pio_usb ${PICO_PIO_USB_PATH}/src/usb_tx.pio)
        pico_generate_pio_header(tinyusb_pico_pio_usb ${PICO_PIO_USB_PATH}/src/usb_rx.pio)
    endif()
endif()

# Main executable
add_executable(ps3-pico-charger
    src/main.c
    src/usb_host.c
)

# Link libraries
target_link_libraries(ps3-pico-charger PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_clocks
    hardware_watchdog
    hardware_pio
    hardware_xosc
    tinyusb_host
    tinyusb_pico_pio_usb
    pico_pio_usb
)

# Include directories
target_include_directories(ps3-pico-charger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler options
target_compile_options(ps3-pico-charger PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Os
)

# TinyUSB MCU selection
target_compile_definitions(ps3-pico-charger PRIVATE
    CFG_TUSB_MCU=OPT_MCU_RP2040
)

# Generate UF2 output (drag-and-drop flashable)
pico_add_extra_outputs(ps3-pico-charger)

# Disable stdio USB/UART (not needed for a charger)
pico_enable_stdio_usb(ps3-pico-charger 0)
pico_enable_stdio_uart(ps3-pico-charger 0)