# PS3 Pico Charger

DualShock 3 / Sixaxis controller charger using a Raspberry Pi Pico.

The PS3 controller won't charge from a dumb 5V source — it requires a USB host handshake (RESET, SET_ADDRESS, GET_DESCRIPTOR) before it enables its charging circuit. This firmware provides that handshake using Pico-PIO-USB and TinyUSB.

> **⚠️ This firmware has been tested on physical hardware but may still have edge cases.** See [Testing Checklist](#testing-checklist) for what's been verified.

## Wiring

```
Pico GP0  ───────────► D+     ┐
Pico GP1  ───────────► D-     │  USB-A Female Connector
Pico VBUS ───────────► VBUS   │  (solder to breakout board)
Pico GND  ───────────► GND    ┘
```

- **22Ω series resistors** on D+ and D- are optional for short wire runs (under ~10cm). They improve signal integrity on longer runs but are not required for direct soldered connections.
- **VBUS** comes from the Pico's micro-USB port (5V from your wall charger)
- **No pull-up resistor needed** on D+ — the DS3 provides its own
- Plugging in a non-DS3 USB device won't harm anything, but it won't do anything useful either

### Parts

| Part                    | Qty | Notes                              |
| ----------------------- | --- | ---------------------------------- |
| Raspberry Pi Pico       | 1   | Any Pico variant works             |
| USB-A female connector  | 1   | Panel-mount or breakout board      |
| 22Ω resistor            | 2   | Optional for short wire runs       |
| Micro-USB cable         | 1   | For power from wall adapter        |
| 5V wall adapter         | 1   | ≥1A recommended                    |

## LED States

| Pattern           | Meaning                            |
| ----------------- | ---------------------------------- |
| Solid ON          | Controller connected and charging  |
| Slow blink (1 Hz) | Powered on, waiting for controller |
| OFF               | Deep sleep (wakes on plug-in)     |

## Deep Sleep

After 10 seconds with no controller connected, the Pico enters dormant mode (~1.9µA RP2040 draw). It wakes automatically when a controller is plugged in (D+ rising edge) and reboots to resume charging.

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
sudo pacman -S cmake arm-none-eabi-gcc arm-none-eabi-newlib git python3

git clone --depth 1 --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
git clone https://github.com/RP2/ps3-pico-charger.git
cd ps3-pico-charger
mkdir build && cd build
export PICO_SDK_PATH=~/pico-sdk
cmake ..
make
```

### Ubuntu / Debian

```bash
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi \
                 libstdc++-arm-none-eabi-newlib python3 git

git clone --depth 1 --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
git clone https://github.com/RP2/ps3-pico-charger.git
cd ps3-pico-charger
mkdir build && cd build
export PICO_SDK_PATH=~/pico-sdk
cmake ..
make
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

| Component    | Role                                           |
| ------------ | ---------------------------------------------- |
| Pico SDK     | Build system & HAL                             |
| TinyUSB      | USB host stack (host-only, zero class drivers) |
| Pico-PIO-USB | Bit-banged USB host on GPIO pins               |

The firmware uses TinyUSB in host-only mode with **all class drivers disabled** (no HID, CDC, MSC). We only need the enumeration handshake — no data communication with the controller.

## Troubleshooting

| Symptom                                | Likely Cause                | Fix                                                                  |
| -------------------------------------- | --------------------------- | -------------------------------------------------------------------- |
| LED never turns on                     | PIO-USB not initializing    | Check that GP0/GP1 are connected correctly (D+ on GP0, D- on GP1)    |
| LED blinks but never goes solid        | DS3 not enumerating         | Check solder joints on D+/D-, try adding 22Ω series resistors        |
| Controller plugged in but not charging | Enumeration failing         | Try a different USB cable between Pico and wall adapter              |
| Pico gets hot                          | Short circuit on D+/D-      | Check for solder bridges between D+ and D- or to VBUS/GND           |
| Pico won't wake from deep sleep        | D+ not pulling high         | Check that GP0 is connected to D+ on the USB-A connector            |

## Testing Checklist

- [x] Compiles without errors
- [x] Pico boots and LED blinks (no controller connected)
- [x] DS3 controller charges when plugged in
- [x] LED turns solid ON when controller is connected
- [x] LED returns to slow blink when controller is disconnected
- [ ] Pico enters deep sleep after 10 seconds idle
- [ ] Pico wakes from deep sleep when controller is plugged in
- [ ] Controller continues charging after wake-from-sleep reboot
- [ ] Works reliably with 22Ω series resistors on D+/D-
- [ ] Works reliably with longer wire runs (>10cm)

## Enclosure

3D-printable enclosure design files and notes are in the [`enclosure/`](enclosure/) directory.

## License

This project is licensed under the [MIT License](LICENSE).

The compiled binary includes third-party code (Pico SDK, TinyUSB, Pico-PIO-USB). See [NOTICE](NOTICE) for attribution and license details.