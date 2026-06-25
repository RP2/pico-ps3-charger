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
| Off               | Deep sleep (power saving mode)     |

After 5 seconds with no controller connected, the Pico enters XOSC dormant deep sleep (~2 mA). Plugging a DS3 into the USB-A port causes its D+ pull-up to wake the Pico, which then re-initializes the USB host and enumerates the controller — LED turns solid ON and charging begins.

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

The build produces `ps3-pico-charger.uf2` — the only file needed for flashing.

## How It Works

1. Pico boots, sets clock to 120 MHz (required by PIO-USB)
2. Initializes PIO-USB on GP0/GP1 and TinyUSB host stack
3. LED blinks slowly while waiting for a controller
4. If no controller is connected after 5 seconds, the Pico enters XOSC dormant deep sleep (~2 mA). D+ (GP0) is configured as a wake pin with an internal pulldown.
5. When a DS3 is plugged into the USB-A port, its internal 1.5 kΩ pull-up brings D+ high, waking the Pico.
6. On wake, clocks are restored and a watchdog reboot triggers a fresh boot.
7. The booted firmware detects the wake-from-sleep indication and performs a forced USB bus reset, then TinyUSB enumeration proceeds:
   - USB RESET (SE0 for 10-20ms)
   - SET_ADDRESS (assigns device address)
   - GET_DESCRIPTOR (reads device descriptor)
8. DS3 sees a valid USB host and enables charging — LED turns solid ON
9. If the controller is unplugged, LED resumes blinking for 5 seconds, then the Pico returns to deep sleep

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

## Testing Checklist

- [x] Compiles without errors
- [x] Pico boots and LED blinks (no controller connected)
- [x] DS3 controller charges when plugged in
- [x] LED turns solid ON when controller is connected
- [x] LED returns to slow blink when controller is disconnected
- [x] Pico enters deep sleep 5 seconds after controller disconnects (LED off)
- [x] Plugging a DS3 into the USB-A port wakes the Pico from deep sleep
- [x] Controller charges after wake from deep sleep
- [x] Controller charges when unplugged and re-plugged during normal operation
- [ ] Works reliably with 22Ω series resistors on D+/D-
- [ ] Works reliably with longer wire runs (>10cm)

## Enclosure

3D-printable enclosure design files and notes are in the [`enclosure/`](enclosure/) directory.

## License

This project is licensed under the [MIT License](LICENSE).

The compiled binary includes third-party code (Pico SDK, TinyUSB, Pico-PIO-USB). See [NOTICE](NOTICE) for attribution and license details.