/*
 * PS3 Pico Charger - USB Host Implementation
 *
 * Initializes Pico-PIO-USB on GP0/GP1 and runs TinyUSB
 * in host-only mode. The DS3 controller starts charging as soon
 * as TinyUSB completes basic USB enumeration (RESET, SET_ADDRESS,
 * GET_DESCRIPTOR). No class drivers are needed.
 *
 * Deep sleep:
 *   Enter XOSC dormant with D+ (GP0) as wake pin.  On a rising edge
 *   (controller plugged in), the XOSC restarts, clocks are restored,
 *   and a watchdog reboot triggers a fresh boot.  After the reboot,
 *   a forced bus reset (via PIO-USB's tuh_rhport_reset_bus) ensures
 *   the DS3's connect edge is reliably detected and enumerated.
 */

#include "usb_host.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "hardware/pll.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "pico/sleep.h"

#include "tusb.h"
#include "pio_usb.h"

//--------------------------------------------------------------------
// State
//--------------------------------------------------------------------

static volatile bool device_mounted = false;
static bool woke_from_deep_sleep = false;

//--------------------------------------------------------------------
// Init
//--------------------------------------------------------------------

void usb_host_init(void)
{
    set_sys_clock_khz(SYS_CLOCK_KHZ, true);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIN_DP;
    pio_cfg.skip_alarm_pool = false;

    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
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
// Wake-Cause Detection
//--------------------------------------------------------------------

void usb_host_check_wake_cause(void)
{
    uint32_t const scratch = watchdog_hw->scratch[DORMANT_WAKE_SCRATCH];
    watchdog_hw->scratch[DORMANT_WAKE_SCRATCH] = 0;
    woke_from_deep_sleep = (scratch == DORMANT_WAKE_MAGIC);
}

bool usb_host_woke_from_deep_sleep(void)
{
    return woke_from_deep_sleep;
}

void usb_host_clear_wake_flag(void)
{
    woke_from_deep_sleep = false;
}

//--------------------------------------------------------------------
// Deep Sleep — XOSC dormant, D+ wake, watchdog reboot
//--------------------------------------------------------------------

// Clock setup for dormant entry, derived from CircuitPython's
// prepare_for_dormant_xosc().  Switches the system to run directly
// from XOSC at 12 MHz and disables the PLLs.
//
// Unlike sleep_run_from_xosc() (pico-extras), this function does
// NOT call setup_default_uart(), so GP0 and GP1 are NOT repurposed
// as UART0 TX/RX — essential for D+ wake detection.
static void prepare_dormant_clocks(void)
{
    uint src_hz = XOSC_MHZ * MHZ;
    uint clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC;

    clock_configure(clk_ref, clk_ref_src, 0, src_hz, src_hz);
    clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, src_hz, src_hz);
    clock_stop(clk_usb);
    clock_stop(clk_adc);
    clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, src_hz, 46875);
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, src_hz, src_hz);
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);
}

// Release PIO-USB from D+/D- and stop all PIO0 state machines.
// We do this manually because pio_usb_host_stop() in Pico-PIO-USB
// 0.7.2 spins forever waiting for a callback flag that is never
// cleared.
static void usb_host_stop_pio_usb(void)
{
    for (int sm = 0; sm < 4; sm++) {
        pio_sm_set_enabled(pio0, sm, false);
    }
    pio_clear_instruction_memory(pio0);

    gpio_init(PIN_DP);
    gpio_set_dir(PIN_DP, GPIO_IN);

    gpio_init(PIN_DM);
    gpio_set_dir(PIN_DM, GPIO_IN);
}

void usb_host_sleep_and_reboot(void)
{
    // LED blink to confirm sleep trigger.
    gpio_put(LED_PIN, 1);
    busy_wait_ms(50);
    gpio_put(LED_PIN, 0);

    // Write wake magic into watchdog scratch.
    watchdog_hw->scratch[DORMANT_WAKE_SCRATCH] = DORMANT_WAKE_MAGIC;

    // Disable interrupts so the PIO-USB alarm timer doesn't fire
    // after we stop the state machines.
    disable_interrupts();
    usb_host_stop_pio_usb();

    // D+ input with pulldown — DS3's 1.5 kΩ pull-up overpowers this
    // when plugged in, creating a rising edge.
    gpio_init(PIN_DP);
    gpio_set_dir(PIN_DP, GPIO_IN);
    gpio_pull_down(PIN_DP);
    gpio_set_input_enabled(PIN_DP, true);

    // Dormant wake IRQ: rising edge on GPIO 0.
    uint32_t const event = GPIO_IRQ_EDGE_RISE;
    gpio_set_dormant_irq_enabled(PIN_DP, event, true);

    // Switch to XOSC 12 MHz, disable PLLs (no UART clobber).
    prepare_dormant_clocks();

    // Halt all clocks until D+ rising edge restarts XOSC.
    xosc_dormant();

    // ===== WAKEN =====
    gpio_set_dormant_irq_enabled(PIN_DP, event, false);
    gpio_acknowledge_irq(PIN_DP, event);

    // Restore clocks and PLLs.
    sleep_power_up();

    // Full CPU reset via watchdog.
    watchdog_reboot(0, 0, 1);
    while (true) {
        tight_loop_contents();
    }
}

//--------------------------------------------------------------------
// Forced Bus Reset (for wake-from-sleep re-enumeration)
//--------------------------------------------------------------------

// Drive a USB bus reset after PIO-USB init to ensure the already-
// connected DS3 generates a clean connect edge for TinyUSB.
void usb_host_force_bus_reset(void)
{
    tuh_rhport_reset_bus(BOARD_TUH_RHPORT, true);
    busy_wait_ms(100);
    tuh_rhport_reset_bus(BOARD_TUH_RHPORT, false);

    busy_wait_ms(500);
}

// Full USB host re-initialisation.  Tears down the current USB host
// stack and PIO-USB, then re-inits from scratch.  Used when the
// wake-from-sleep enumeration times out.
void usb_host_reinit(void)
{
    disable_interrupts();
    tuh_deinit(BOARD_TUH_RHPORT);
    usb_host_stop_pio_usb();
    set_sys_clock_khz(SYS_CLOCK_KHZ, true);
    usb_host_init();
    usb_host_force_bus_reset();
}

//--------------------------------------------------------------------
// TinyUSB Host Callbacks
//--------------------------------------------------------------------

void tuh_mount_cb(uint8_t dev_addr)
{
    (void)dev_addr;
    device_mounted = true;
    gpio_put(LED_PIN, 1);
}

void tuh_umount_cb(uint8_t dev_addr)
{
    (void)dev_addr;
    device_mounted = false;
    gpio_put(LED_PIN, 0);
}
