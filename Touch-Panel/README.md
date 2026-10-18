# ESP32 Touchscreen Control Panel

An ESP32-based touchscreen interface for monitoring a local server and exploring embedded user interface design.

The project uses a 2.4-inch TFT display with a resistive touchscreen to provide an interactive control panel with multiple menus, customizable themes, and server statistics retrieved over Wi-Fi.

## Features

- Interactive touchscreen menu
- Multiple user interface themes
- Adjustable screen brightness
- Configurable automatic refresh intervals
- Server statistics retrieved through a local HTTP API
- Wi-Fi connectivity
- Persistent settings using ESP32 Preferences
- Expandable interface for future features

## Hardware

| Component | Description |
|---|---|
| Microcontroller | ESP32 development board |
| Display | 2.4-inch TFT, 240 × 320 |
| Display controller | ILI9341 |
| Touch controller | XPT2046 |
| Communication | Wi-Fi / HTTP |

## Software and Libraries

- Arduino framework for ESP32
- TFT_eSPI
- XPT2046_Touchscreen
- WiFi
- HTTPClient
- ArduinoJson
- Preferences

## Server Integration

The control panel communicates with a local server through an HTTP API.

The server endpoint returns JSON data, including:

- Hostname and server status
- Uptime
- RAM usage
- Disk usage
- Running and total Docker containers
- System load average

The API address is configured in the firmware and should be changed to match your own server.

## User Interface

The interface includes the following menu sections:

- **Server** — server statistics
- **Lamp** — reserved for future functionality
- **Sensors** — reserved for future functionality
- **Network** — reserved for future functionality
- **Storage** — reserved for future functionality
- **Room** — reserved for future functionality
- **Tools** — reserved for future functionality
- **Settings** — display themes, brightness, and refresh interval

Some menu sections are placeholders for future development.

## Setup

1. Install the Arduino IDE.
2. Install the ESP32 board support package.
3. Install the required libraries listed above.
4. Open `TouchPanel.ino` in the Arduino IDE.
5. Configure your Wi-Fi credentials locally.
6. Configure the server API endpoint to match your network.
7. Select the correct ESP32 board and port.
8. Upload the firmware.

**Important:** Never publish Wi-Fi passwords, API tokens, or other private credentials. Use your own local configuration before uploading the firmware.

## Project Status

**Ongoing development**

The basic display and touchscreen functionality is working. The interface and server monitoring features are being developed and expanded.

## Future Improvements

- Add more interactive controls
- Implement sensor integration
- Expand network and system monitoring
- Improve the interface and add project documentation
- Add photographs and demonstrations

## License

No license has been selected yet.
