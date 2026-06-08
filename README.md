# PS3 Pico Charger

DualShock 3 / Sixaxis controller charger using a Raspberry Pi Pico.

The PS3 controller won't charge from a dumb 5V source — it requires a USB host handshake (RESET, SET_ADDRESS, GET_DESCRIPTOR) before it enables its charging circuit. This firmware provides that handshake using Pico-PIO-USB and TinyUSB.

## Wiring

```
Pico GP0  ──[22Ω]──► D+   ┐
Pico GP1  ──[22Ω]──► D-   │  USB-A Female Connector
Pico VBUS ───────────► VBUS │  (solder to breakout board)
Pico GND  ───────────► GND  ┘
```

- **22Ω resistors** on D+ and D- lines (required for USB signal integrity)
- **VBUS** comes from the Pico's micro-USB port (5V from your wall charger)
- **No pull-up resistor needed** on D+ — the DS3 provides its own

### Parts

| Part | Qty | Notes |
|---|---|---|
| Raspberry Pi Pico | 1 | Any Pico variant works |
| USB-A female connector | 1 | Panel-mount or breakout board |
| 22Ω resistor | 2 | 0805 SMD or 1/4W through-hole |
| Micro-USB cable | 1 | For power from wall adapter |
| 5V wall adapter | 1 | ≥1A recommended |

## LED States

| Pattern | Meaning |
|---|---|
| Solid ON | Controller connected and charging |
| Slow blink (1 Hz) | Powered on, waiting for controller |
| OFF | Deep sleep (near-zero power draw) |

## Deep Sleep

After 10 seconds with no controller connected, the Pico enters dormant mode (~1.9μA). It wakes automatically when a controller is plugged in (D+ rising edge) and reboots to resume charging.

## Flashing

1. Hold **BOOTSEL** on the Pico
2. Plug into computer via micro-USB
3. Release BOOTSEL — Pico mounts as a USB drive
4. Copy `ps3-pico-charger.uf2` to the drive
5. Pico reboots with the new firmware

## Building from Source

You need the Pico SDK cloned separately (it's not included in this repo).

### Arch Linux

```bash
# Install toolchain
sudo pacman -S cmake arm-none-eabi-gcc arm-none-eabi-newlib git python3

# Clone Pico SDK (one-time, shared across projects)
git clone --depth 1 --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk

# Clone and build
git clone https://github.com/RP2/ps3-pico-charger.git
cd ps3-pico-charger
mkdir build && cd build
export PICO_SDK_PATH=~/pico-sdk
cmake ..
make
# Output: ps3-pico-charger.uf2
```

### macOS

```bash
brew install cmake gcc-arm-embedded
git clone --depth 1 --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
git clone https://github.com/RP2/ps3-pico-charger.git
cd ps3-pico-charger
mkdir build && cd build
export PICO_SDK_PATH=~/pico-sdk
cmake ..
make
```

## How It Works

1. Pico boots, sets clock to 120 MHz (required by PIO-USB)
2. Initializes PIO-USB on GP0/GP1 and TinyUSB host stack
3. When a DS3 is plugged in, TinyUSB performs USB enumeration:
   - USB RESET (SE0 for 10-20ms)
   - SET_ADDRESS (assigns device address)
   - GET_DESCRIPTOR (reads device descriptor)
4. DS3 sees a valid USB host and enables charging
5. LED turns solid ON
6. If no controller for 10 seconds → deep sleep → wake on reconnect

## Architecture

| Component | Role |
|---|---|
| Pico SDK | Build system & HAL |
| TinyUSB | USB host stack (host-only, zero class drivers) |
| Pico-PIO-USB | Bit-banged USB host on GPIO pins |

The firmware uses TinyUSB in host-only mode with **all class drivers disabled** (no HID, CDC, MSC). We only need the enumeration handshake — no data communication with the controller.

## License

[MIT](LICENSE)