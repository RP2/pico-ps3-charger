/*
 * PS3 Pico Charger - TinyUSB Configuration
 *
 * Host-only, zero class drivers, single port.
 * We only need the USB host to enumerate the DS3 controller
 * so it starts charging. No HID, CDC, MSC, or any other
 * class driver is required.
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#ifndef CFG_TUH_MEM_SECTION
#define CFG_TUH_MEM_SECTION
#endif

#ifndef CFG_TUH_MEM_ALIGN
#define CFG_TUH_MEM_ALIGN     __attribute__((aligned(4)))
#endif

//--------------------------------------------------------------------
// Host Configuration
//--------------------------------------------------------------------

// Host stack only (no device mode)
#define CFG_TUH_ENABLED       1
#define CFG_TUD_ENABLED       0

// Use PIO-USB as host controller on GPIO pins
#define CFG_TUH_RPI_PIO_USB   1

// Roothub port 1 = PIO-USB (port 0 = native USB, not used)
#define BOARD_TUH_RHPORT      1

// Full-speed only (12 Mbps) - all DS3 needs
#define CFG_TUH_MAX_SPEED     OPT_MODE_FULL_SPEED

//--------------------------------------------------------------------
// Driver Configuration - MINIMAL
//--------------------------------------------------------------------

// Small enumeration buffer (DS3 descriptor is small)
#define CFG_TUH_ENUMERATION_BUFSIZE  256

// No hub support needed (single direct port)
#define CFG_TUH_HUB                 0

// No class drivers - we only enumerate, don't communicate
#define CFG_TUH_CDC                 0
#define CFG_TUH_CDC_FTDI            0
#define CFG_TUH_CDC_CP210X          0
#define CFG_TUH_CDC_CH34X           0
#define CFG_TUH_HID                 0
#define CFG_TUH_MSC                 0
#define CFG_TUH_VENDOR              0

// Max 1 device (single port)
#define CFG_TUH_DEVICE_MAX          1

// Minimal endpoints
#define CFG_TUH_ENDPOINT_MAX        4

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */