/*
 * PS3 Pico Charger - USB Host Configuration
 *
 * Single port: GP0 (D+) / GP1 (D-)
 * D- must be the next GPIO after D+ per PIO-USB convention.
 */

#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------
// Port Configuration
//--------------------------------------------------------------------

// USB host port pins (D- must be D+ + 1)
#define PIN_DP   0   // D+ on GP0
#define PIN_DM   1   // D- on GP1

// Built-in LED on Pico (GP25)
#define LED_PIN   25

// System clock must be a multiple of 12 MHz for PIO-USB
#define SYS_CLOCK_KHZ  120000

// Seconds with no device before entering deep sleep
#define SLEEP_TIMEOUT_SEC  10

//--------------------------------------------------------------------
// API
//--------------------------------------------------------------------

// Initialize PIO-USB and TinyUSB host stack. Call once at boot.
void usb_host_init(void);

// Process USB host events. Call repeatedly in main loop.
void usb_host_task(void);

// Returns true if a device is currently mounted (charging).
bool usb_host_device_connected(void);

#endif /* USB_HOST_H */