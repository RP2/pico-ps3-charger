/*
 * PS3 Pico Charger - Main Entry Point
 *
 * DualShock 3 / Sixaxis controller charger using Raspberry Pi Pico.
 * Uses Pico-PIO-USB to create a USB host port on GP0/GP1.
 * The DS3 requires a USB host handshake before it will charge;
 * this firmware provides that handshake via TinyUSB's host stack.
 *
 * LED states:
 *   Solid ON  = Controller connected and charging
 *   Slow blink = Powered, waiting for controller
 *   OFF       = Deep sleep (near-zero power draw)
 *
 * Deep sleep:
 *   After SLEEP_TIMEOUT_SEC seconds with no device connected,
 *   the Pico enters dormant mode (~1.9uA). It wakes when a
 *   controller is plugged in (D+ rising edge) and reboots.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/structs/syscfg.h"
#include "hardware/xosc.h"
#include "hardware/watchdog.h"

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
// Deep Sleep (Dormant Mode)
//--------------------------------------------------------------------

// Enter dormant mode. Wakes when D+ goes high (controller plugged in).
// After wake, we reboot the Pico to reinitialize everything cleanly.
static void enter_deep_sleep(void)
{
    // Turn off LED
    gpio_put(LED_PIN, 0);

    // Deinitialize USB host (stop PIO state machines, release pins)
    usb_host_deinit();

    // Configure D+ pin as input with pull-down.
    // When a DS3 is plugged in, its internal 1.5kΩ pull-up
    // on D+ pulls the line high, triggering wake.
    gpio_init(PIN_DP);
    gpio_set_dir(PIN_DP, GPIO_IN);
    gpio_pull_down(PIN_DP);

    // Small delay for GPIO to settle
    sleep_ms(50);

    // TODO: Verify this is the correct register for dormant wake.
    // proc_in_sync_bypass bypasses the input synchronizer, which may
    // be needed for GPIO wake from dormant mode. If wake doesn't work
    // during testing, try using gpio_set_irq_enabled() with
    // GPIO_IRQ_LEVEL_HIGH instead, or check the RP2040 datasheet
    // section 2.16.2 for the correct dormant wake mechanism.
    hw_set_bits(&syscfg_hw->proc_in_sync_bypass, 1u << PIN_DP);

    // Enter dormant mode - XOSC stops, CPU halts.
    // Wakes when D+ goes high (GPIO event).
    xosc_dormant();

    // --- WOKE UP HERE ---
    // After waking from dormant, the system is in a partially
    // initialized state. The simplest and most reliable approach
    // is to reboot the Pico completely.
    watchdog_reboot(0, 0, 0);

    // Should never reach here
    while (1) { tight_loop_contents(); }
}

//--------------------------------------------------------------------
// Main
//--------------------------------------------------------------------

int main(void)
{
    // Initialize USB host (sets clock to 120 MHz)
    usb_host_init();

    // Track time of last disconnect for sleep timeout
    absolute_time_t idle_start = get_absolute_time();
    bool was_connected = false;

    while (true) {
        // Process USB host events
        usb_host_task();

        bool connected = usb_host_device_connected();

        if (connected) {
            // Device is charging - LED is solid ON (set by mount callback)
            idle_start = get_absolute_time();
            was_connected = true;
        } else {
            // No device - check if we should enter deep sleep
            if (was_connected) {
                // Just disconnected - reset timer
                idle_start = get_absolute_time();
                was_connected = false;
            }

            // Slow blink while waiting
            led_slow_blink();

            // Check sleep timeout
            int64_t elapsed_sec = absolute_time_diff_us(idle_start, get_absolute_time()) / 1000000;
            if (elapsed_sec >= SLEEP_TIMEOUT_SEC) {
                enter_deep_sleep();
                // After reboot, we start fresh from main()
            }
        }

        // Small delay to avoid busy-waiting
        sleep_ms(5);
    }

    return 0;
}