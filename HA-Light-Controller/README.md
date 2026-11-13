# Home Assistant Light Controller

A small physical controller for a smart light, built with an ESP32-C3, a rotary encoder and a push button. It talks to Home Assistant through ESPHome.

- **Button** turns the light on and off.
- **Turning the encoder** changes the brightness smoothly (5% per detent).
- **Pressing the encoder** toggles between warm white (2000 K) and cool white (6500 K).
- **Status LED** (optional) is wired to GPIO3, but this firmware version does not drive it yet.

## Hardware

| Component | Description |
|---|---|
| Microcontroller | ESP32-C3 Mini (LOLIN C3 Mini) |
| Rotary encoder | KY-040 module (labelled `5V`, `KEY`, `S1`, `S2`, `GND` on some boards) |
| Button | 4-pin tact switch |
| Status LED (optional) | Any LED with a 330 Ω resistor |

The controlled light must support brightness and color temperature (`color_temp` mode) in Home Assistant.

## Wiring

| Part | Pin on part | ESP32-C3 pin |
|---|---|---|
| KY-040 | `+` / `5V` | 3V3 |
| KY-040 | `GND` | GND |
| KY-040 | `CLK` / `S1` | GPIO4 |
| KY-040 | `DT` / `S2` | GPIO5 |
| KY-040 | `SW` / `KEY` | GPIO6 |
| Button | one pin | GPIO10 |
| Button | pin on the opposite side | GND |
| LED (optional) | anode, through 330 Ω | GPIO3 |
| LED (optional) | cathode | GND |

Notes:

- Power the KY-040 from **3V3**, not 5 V. Its on-board pull-up resistors would otherwise put 5 V on the ESP32 inputs.
- No external resistors are needed for the inputs. They use the internal pull-ups.
- On a 4-pin tact switch the pins are joined in pairs inside the switch. Use one pin from each pair, or the switch will look permanently pressed. A continuity check with a multimeter shows which pins are paired.
- GPIO2, GPIO8 and GPIO9 are strapping pins on the ESP32-C3, so they are not used. Check the pin labels on your own board.
- If turning the knob clockwise darkens the light, swap the `S1` and `S2` wires (or the two pins in the YAML).

## Software

- [ESPHome](https://esphome.io/) (the native Home Assistant API with encryption)
- Home Assistant

The firmware is a single file: [`esphome/light-controller.yaml`](./esphome/light-controller.yaml).

Rotation is collected locally and sent to Home Assistant every 120 ms as one `brightness_step_pct` call. This keeps fast turning smooth and avoids flooding Home Assistant with service calls.

## Setup

1. Install ESPHome (`pip install esphome`).
2. Copy `esphome/secrets.example.yaml` to `esphome/secrets.yaml` and fill in your own values (Wi-Fi, API key, passwords). This file is ignored by git. The second Wi-Fi network is optional: delete it from both files if you only have one.
3. In `esphome/light-controller.yaml`, set `light_entity` to your own light entity.
4. Check that the board and pins match your hardware.
5. Connect the ESP32-C3 over USB and flash it:

   ```
   cd esphome
   esphome run light-controller.yaml
   ```

6. In Home Assistant, open **Settings → Devices & services**, add the discovered ESPHome device and enter the API key from `secrets.yaml`.
7. Open the ESPHome integration options for the device and enable **Allow the device to perform Home Assistant actions**. Without this the ESP cannot control the light.

### Windows tip

The ESP-IDF toolchain can fail with `fatal error: bits/c++config.h: No such file or directory` because of the 260-character Windows path limit. Setting a short tools directory fixes it:

```
$env:ESPHOME_ESP_IDF_PREFIX = "C:\esp"
```

Delete the project's `.esphome/build` folder after changing it.

### Wi-Fi tip

The firmware lists two networks and tries them in order. If you replace or move your router, add a mesh node/extender with a different SSID, or rename the network, the controller keeps working as long as one of the listed networks is in range.

If the controller ever drops off Home Assistant, read its logs over USB (`esphome logs light-controller.yaml --device COM3`). Repeated `No networks found` for your SSID means the network is not visible from where the board stands (the SSID changed, or its 2.4 GHz radio is off). The ESP32-C3 supports 2.4 GHz only.

**Important:** never commit `secrets.yaml`. Wi-Fi passwords, API keys and tokens stay local.

## Project Status

**Completed.** All controls work. At the edge of Wi-Fi range the controller can occasionally lag.

## Possible Improvements

- Hold the encoder and turn it to change color temperature smoothly
- Add a case or a 3D-printed enclosure
- Support several lights or scenes
