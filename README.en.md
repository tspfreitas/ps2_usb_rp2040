# PS1/PS2 -> USB (RP2040-Zero), 2 controllers

[Português](README.md) | **English**

USB adapter for **2 simultaneous PS1/PS2 controllers** using an RP2040 (RP2040-Zero or Pico).
Each port shows up on the PC as a separate HID gamepad (recognized in `joy.cpl`, in games and in
the browser). Use it for playing, or as a test/repair bench for controllers.

- Supports digital PS1 pads, DualShock 1 and 2 (analog) and the PS1 analog joystick.
- DualShock pads are switched to analog mode automatically; on the DualShock 2 the L2/R2 pressure is read.
- Rumble test: **SELECT + START + L1** (small motor) and **SELECT + START + R1** (large motor).
- License: MIT.

> Status: firmware written and syntax-checked, but **not yet tested on hardware**.

## Wiring

Both ports share DATA, CMD, CLK, 3V3 and GND. Only ATT is separate (like on the console).

| Connector (pin) | Function | RP2040-Zero |
|---|---|---|
| 1 (DATA, brown) | controller output | GP4 + **1 kΩ resistor to 3V3** (required) |
| 2 (CMD, orange) | controller input | GP3 |
| 3 (motors, gray) | 7.2 V rumble supply | 5V/VBUS (test only, weak) or unconnected |
| 4 (GND, black) | GND | GND |
| 5 (VCC, red) | power | **3V3** (never 5 V) |
| 6 (ATT, yellow) | select | GP5 (port 1) / GP6 (port 2) |
| 7 (CLK, blue) | clock | GP2 |
| 8 | not used | – |
| 9 (ACK, green) | acknowledge | not used |

Check your connector's pin order with a multimeter before powering it.
Pins are defined in `psx.h` (SCK/CMD/DAT must be SPI0 pins: GP2/GP3/GP4 or GP18/GP19/GP16).

### Wiring diagrams

Raspberry Pi Pico:

![Wiring to the Raspberry Pi Pico](docs/wiring_pico_en.png)

RP2040-Zero (optional):

![Wiring to the RP2040-Zero](docs/wiring_rp2040_zero_en.png)

Vector versions (SVG) are in `docs/`.

## Building the .uf2

### Option A: GitHub Actions (nothing to install)

1. Create a repository and push these files (including `.github/`).
2. Open the Actions tab and run "build-uf2" (or just push).
3. Download the `ps2_usb_rp2040-uf2` artifact; it contains `ps2_usb_rp2040.uf2`.

### Option B: local (Linux/WSL)

```
sudo apt install cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
export PICO_SDK_PATH=~/pico-sdk
cmake -B build -S .
cmake --build build -j
```

The file ends up in `build/ps2_usb_rp2040.uf2`.

## Flashing

Hold the BOOT button on the RP2040-Zero, plug in the USB cable and release it. The board shows up as a
drive (RPI-RP2). Copy the `.uf2` onto it; the board reboots by itself.

## Mapping

Same order as the browser *Standard Gamepad* and the layout emulators expect (0-based indexes, like in
hardwaretester and SDL; in `joy.cpl` buttons start at 1).

| Index | Button | Index | Button |
|---|---|---|---|
| 0 | ✕ (A) | 9 | Start |
| 1 | ○ (B) | 10 | L3 |
| 2 | □ (X) | 11 | R3 |
| 3 | △ (Y) | 12 | D-pad ↑ |
| 4 | L1 | 13 | D-pad ↓ |
| 5 | R1 | 14 | D-pad ← |
| 6 | L2 | 15 | D-pad → |
| 7 | R2 | 16 | **Mode / Analog** (pulse) |
| 8 | Select | | |

- **Sticks:** left = X/Y axes, right = Z/Rz axes. Slider/Dial = analog L2/R2 pressure
  (DualShock 2; on other pads it is 0 or 255).
- **D-pad:** sent as 4 buttons (12–15) and also as a hat/POV, so it works in emulators and in older games.
  To use only one: `PAD_DPAD_AS_BUTTONS` / `PAD_DPAD_AS_HAT` in `main.c`.
- **DualShock ANALOG button:** the firmware leaves the mode unlocked. Pressing ANALOG makes the pad fall back to
  digital, the adapter switches it back to analog right away and sends a pulse on **button 16 (Mode)**. In the
  emulator, bind its "Analog" (toggle analog mode) action to that button. To lock analog mode as before:
  `PSX_LOCK_ANALOG 1` in `psx.h`.

## Browser test (hardwaretester.com/gamepad)

The adapter is a generic HID gamepad ("Mapping" = n/a): the browser shows buttons and axes by index, but may not draw
the standard console layout (that depends on the browser knowing the VID/PID).
Press a button on the controller first, because browsers only list a gamepad after the first input.

## Status LED (RP2040-Zero)

The on-board RGB LED (GP16) shows the connection to the computer:

| Color | Meaning |
|---|---|
| Solid green | Connected to the computer (USB enumerated), no controller detected |
| Solid blue | Connected to the computer and at least 1 PS1/PS2 controller detected |
| Solid yellow | USB suspended (computer asleep) |
| Blinking red | Board powered, but the computer has not enumerated the USB device yet |

Options in `led.h`: `STATUS_LED_LEVEL` (brightness 0–255), `STATUS_LED_SHOW_PADS` (0 = always green),
`STATUS_LED_MODE` (0 = no LED, 1 = RP2040-Zero WS2812, 2 = plain LED, e.g. Pico on GP25).

## Emulators (DuckStation, PCSX2)

The adapter is a generic HID gamepad. SDL (used by DuckStation) has no ready-made profile for it, so
**automatic mapping fails** ("No generic mapping was generated"). This is expected: bind the buttons manually,
one by one, in the controller settings, using the table above. For each port pick the device
(`SDL-0` / `SDL-1`) and press a button on the controller so it gets detected.

Optional: a line for SDL's `gamecontrollerdb.txt` (replace `<GUID>` with the GUID your program shows; **untested**):

```
<GUID>,PS1/PS2 Controller,a:b0,b:b1,x:b2,y:b3,leftshoulder:b4,rightshoulder:b5,lefttrigger:b6,righttrigger:b7,back:b8,start:b9,leftstick:b10,rightstick:b11,dpup:b12,dpdown:b13,dpleft:b14,dpright:b15,guide:b16,leftx:a0,lefty:a1,rightx:a2,righty:a3,platform:Windows,
```

## Name and order of the 2 controllers

Both controllers are interfaces of the same USB device (same VID/PID). Windows gives **the same name** to
every device with the same VID/PID, so in `joy.cpl` and SDL both show up with the same name. That is why the
name is now neutral ("PS1/PS2 Controller"), without "Player 1/2". Port 1 is interface 0 and port 2 is
interface 1; the order in which the system lists them can vary, so identify them by pressing a button.

> The PID changed to `0x5054`: Windows installs it as a new device and stops using the old saved name.

## Tuning

- `PSX_BYTE_GAP_US` (psx.h): delay between bytes. If the pad stays digital-only or is unstable, raise it to 30–40.
- `PSX_BAUD` (psx.h): bus clock. With long wires or noise, lower it to 125000.
- `USB_VID` / `USB_PID` (usb_descriptors.c): TinyUSB test values (`0xCafe:0x5054`). Change them if you distribute the adapter.
- `PSX_LOCK_ANALOG`, `PSX_MODE_PULSE_MS`, `PSX_MODE_SETTLE_MS` (psx.h): lock analog mode, Mode button pulse length and delay before switching back to analog.
- `PAD_DPAD_AS_BUTTONS`, `PAD_DPAD_AS_HAT` (main.c): how the D-pad is sent.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing shows up in `joy.cpl` | Missing 1 kΩ pull-up on DATA, VCC not at 3.3 V, CMD/DATA swapped |
| Buttons only, no analog | Pad in digital mode (ID 0x41); raise `PSX_BYTE_GAP_US` |
| Random inputs | Long wires or no common GND; lower `PSX_BAUD` |
| Weak rumble | Pin 3 at 5 V; the original motor uses ~7.2 V |

## Protocol

SPI mode 3 (clock idles high), LSB first, ~250 kHz. Poll: `01 42 00 [motor1] [motor2] ...`;
reply: `FF, ID, 5A, data`. IDs: `41` digital, `73` analog, `79` analog with pressure, `F3` config mode.
Buttons are active-low (0 = pressed).
