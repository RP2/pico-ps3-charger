/*
 * PS3 Pico Charger - Main Entry Point
 *
 * DualShock 3 / Sixaxis controller charger using Raspberry Pi Pico.
 * Uses Pico-PIO-USB to create a USB host port on GP0/GP1.
 * The DS3 requires a USB host handshake before it will charge;
 * this firmware provides that handshake via TinyUSB's host stack.
 *
 * Deep sleep: XOSC dormant with D+ rising-edge wake, then watchdog
 * reboot.  On wake-from-sleep a forced bus reset via PIO-USB ensures
 * reliable re-enumeration of the already-connected DS3.
 *
 * LED states:
 *   Solid ON   = Controller connected and charging
 *   Slow blink = Powered, waiting for controller
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "usb_host.h"

//--------------------------------------------------------------------
// LED Blink
//--------------------------------------------------------------------

#define BLINK_INTERVAL_MS  500

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
    usb_host_check_wake_cause();

    // Initialize USB host (sets clock to 120 MHz)
    usb_host_init();

    // If we just woke from deep sleep, the DS3 is already connected.
    // Force a USB bus reset to generate a clean connect edge for
    // TinyUSB enumeration.
    if (usb_host_woke_from_deep_sleep()) {
        usb_host_force_bus_reset();
    }

    absolute_time_t unplug_time = get_absolute_time();
    bool was_connected = false;

    while (true) {
        usb_host_task();

        if (usb_host_device_connected()) {
            was_connected = true;
            if (usb_host_woke_from_deep_sleep()) {
                usb_host_clear_wake_flag();
            }
        } else {
            if (was_connected) {
                was_connected = false;
                unplug_time = get_absolute_time();
            }

            if (usb_host_woke_from_deep_sleep()) {
                // Just woke — wait for enumeration without blinking.
                static absolute_time_t wake_start = 0;
                static bool wake_timer_init = false;

                if (!wake_timer_init) {
                    wake_start = get_absolute_time();
                    wake_timer_init = true;
                } else if (absolute_time_diff_us(wake_start, get_absolute_time()) >=
                           5000000) {
                    // 5 s with no device — likely a false wake.
                    // Re-init USB host to give the DS3 a fresh
                    // connect edge, then fall through to normal
                    // blink/sleep operation.
                    usb_host_clear_wake_flag();
                    usb_host_reinit();
                    unplug_time = get_absolute_time();
                    wake_timer_init = false;
                }
            } else {
                led_slow_blink();

                if (absolute_time_diff_us(unplug_time, get_absolute_time()) >=
                    SLEEP_TIMEOUT_MS * 1000) {
                    usb_host_sleep_and_reboot();
                }
            }
        }

        sleep_ms(5);
    }

    return 0;
}
