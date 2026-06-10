/*
 * PS3 Pico Charger - Main Entry Point
 *
 * DualShock 3 / Sixaxis controller charger using Raspberry Pi Pico.
 * Uses Pico-PIO-USB to create a USB host port on GP0/GP1.
 * The DS3 requires a USB host handshake before it will charge;
 * this firmware provides that handshake via TinyUSB's host stack.
 *
 * LED states:
 *   Solid ON   = Controller connected and charging
 *   Slow blink  = Powered, waiting for controller
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "usb_host.h"

//--------------------------------------------------------------------
// LED Blink
//--------------------------------------------------------------------

#define BLINK_INTERVAL_MS  500  // 1 Hz blink (500ms on, 500ms off)

static absolute_time_t last_blink_time = 0;
static bool led_on = false;

static void led_slow_blink(void)
{
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_blink_time, now) >= BLINK_INTERVAL_MS * 1000) {
        led_on = !led_on;
        gpio_put(LED_PIN, led_on);
        last_blink_time = now;
    }
}

//--------------------------------------------------------------------
// Main
//--------------------------------------------------------------------

int main(void)
{
    // Initialize USB host (sets clock to 120 MHz)
    usb_host_init();

    while (true) {
        // Process USB host events
        usb_host_task();

        if (usb_host_device_connected()) {
            // Device is charging - LED is solid ON (set by mount callback)
        } else {
            // No device - slow blink while waiting
            led_slow_blink();
        }

        // Small delay to avoid busy-waiting
        sleep_ms(5);
    }

    return 0;
}