/*
 * PS3 Pico Charger - USB Host Configuration
 *
 * Single port: GP0 (D+) / GP1 (D-)
 * D- must be the next GPIO after D+ per PIO-USB convention.
 *
 * Power saving:
 *   After 10 s idle, enter XOSC dormant with D+ configured as wake
 *   pin.  On plug-in the XOSC restarts, clocks are restored, and a
 *   watchdog reboot triggers a fresh boot.  The wake cause is
 *   recorded in a watchdog scratch register.  After the reboot, a
 *   forced bus reset (via PIO-USB) ensures reliable enumeration.
 */

#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------
// Port Configuration
//--------------------------------------------------------------------

#define PIN_DP   0   // D+ on GP0 (D- must be D+ + 1)
#define PIN_DM   1   // D- on GP1
#define LED_PIN  25  // Built-in LED

#define SYS_CLOCK_KHZ  120000  // PIO-USB requires multiple of 12 MHz
#define SLEEP_TIMEOUT_MS  5000

// Watchdog scratch register for wake-from-sleep indication
#define DORMANT_WAKE_MAGIC    0x5A5A0001
#define DORMANT_WAKE_SCRATCH  0

//--------------------------------------------------------------------
// API
//--------------------------------------------------------------------

void usb_host_init(void);
void usb_host_task(void);
bool usb_host_device_connected(void);

void usb_host_check_wake_cause(void);
bool usb_host_woke_from_deep_sleep(void);
void usb_host_clear_wake_flag(void);

// Enter XOSC dormant with D+ rising-edge wake.  After wake, restore
// clocks and trigger a watchdog reboot.  Does not return.
void usb_host_sleep_and_reboot(void);

// Force a USB bus reset (SE0) via the PIO-USB host controller.
// Used on wake-from-sleep to trigger reliable re-enumeration.
void usb_host_force_bus_reset(void);

// Full USB host re-initialisation.  Tears down TinyUSB + PIO-USB
// and re-inits from scratch.  Used when wake enumeration times out.
void usb_host_reinit(void);

#endif /* USB_HOST_H */
