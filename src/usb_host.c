/*
 * PS3 Pico Charger - USB Host Implementation
 *
 * Initializes Pico-PIO-USB on GP0/GP1 and runs TinyUSB
 * in host-only mode. The DS3 controller starts charging as soon
 * as TinyUSB completes basic USB enumeration (RESET, SET_ADDRESS,
 * GET_DESCRIPTOR). No class drivers are needed.
 */

#include "usb_host.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "tusb.h"
#include "pio_usb.h"

//--------------------------------------------------------------------
// State
//--------------------------------------------------------------------

static volatile bool device_mounted = false;

//--------------------------------------------------------------------
// Init
//--------------------------------------------------------------------

void usb_host_init(void)
{
    // PIO-USB requires system clock to be a multiple of 12 MHz.
    // Default 125 MHz is NOT a multiple of 12. Use 120 MHz.
    set_sys_clock_khz(SYS_CLOCK_KHZ, true);

    // Configure LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Configure PIO-USB with default settings (GP0/GP1)
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIN_DP;
    pio_cfg.skip_alarm_pool = false;

    // Pass PIO-USB config to TinyUSB before init
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    // Initialize TinyUSB host stack
    tuh_init(BOARD_TUH_RHPORT);
}

//--------------------------------------------------------------------
// Task
//--------------------------------------------------------------------

void usb_host_task(void)
{
    tuh_task();
}

//--------------------------------------------------------------------
// Device Detection
//--------------------------------------------------------------------

bool usb_host_device_connected(void)
{
    return device_mounted;
}

//--------------------------------------------------------------------
// TinyUSB Host Callbacks
//--------------------------------------------------------------------

// Called when a device is mounted (enumerated).
// Once TinyUSB finishes enumeration, the DS3 sees a valid USB
// host and enables its charging circuit.
void tuh_mount_cb(uint8_t dev_addr)
{
    (void)dev_addr;
    device_mounted = true;
    gpio_put(LED_PIN, 1);
}

// Called when a device is unmounted (disconnected).
void tuh_umount_cb(uint8_t dev_addr)
{
    (void)dev_addr;
    device_mounted = false;
    gpio_put(LED_PIN, 0);
}

//--------------------------------------------------------------------
// Deinit (for deep sleep)
//--------------------------------------------------------------------

void usb_host_deinit(void)
{
    // Mark as disconnected and turn off LED before entering deep sleep.
    // We don't fully deinitialize TinyUSB or PIO here because we'll
    // reboot on wake anyway. The main loop stops calling tuh_task()
    // before entering sleep, so the bus is effectively idle.
    device_mounted = false;
    gpio_put(LED_PIN, 0);
}