# ATSU - Affordable Thermal Sensing Unit

A feature-rich thermal imaging camera firmware for the M5StickC Plus2 with M5Stack Thermal2 module.

## Features

- **Real-time Thermal Imaging**: 32x24 thermal sensor with 32Hz refresh rate
- **Multiple Color Schemes**: 
  - Grayscale
  - Iron (black → red → yellow → white)
  - Rainbow (blue → cyan → green → yellow → red)
- **Night Mode**: Low brightness with LED flashlight functionality
- **WiFi Connectivity**: Connect to your network or create a hotspot
- **Web Interface**: View thermal camera feed remotely via browser
- **Auto Power-off**: Configurable automatic shutdown to save battery
- **LED Indicators**: Visual feedback via thermal module and device LEDs
- **Orientation Detection**: Auto-rotating text based on device orientation
- **Temperature Statistics**: Real-time display of average, max, and min temperatures

## Hardware Requirements

To build your own ATSU, you will need:

1. **M5StickC Plus2** - ESP32-based development board
2. **M5Stack Thermal2 Module** - MLX90640 thermal imaging sensor
3. **Grove Cable** - Included with the Thermal2 module for connecting the devices

## Installation

### Prerequisites

- Arduino IDE or PlatformIO
- Required Libraries:
  - [M5Unified](https://github.com/m5stack/M5Unified)
  - [M5Unit-Thermal2](https://github.com/m5stack/M5Unit-Thermal2)

### Setup Instructions

1. **Hardware Assembly**:
   - Connect the M5Stack Thermal2 module to the M5StickC Plus2 using the Grove cable

2. **Software Installation**:
   - Install Arduino IDE or PlatformIO
   - Install the required libraries (M5Unified and M5Unit-Thermal2)
   - Clone this repository or download the `M5StickCPlus2_Thermal2` source file
   - Open the project in your IDE

3. **WiFi Configuration** (Optional):
   - Edit lines 68-69 in the source code to set your WiFi credentials:
     ```cpp
     static const char *WIFI_SSID = "YourSSID";
     static const char *WIFI_PASS = "YourPassword";
     ```

4. **Flash Firmware**:
   - Connect your M5StickC Plus2 via USB
   - Compile and upload the firmware to the device

## Controls

### Button Layout
- **Button A** (Side button): Middle button
- **Button B** (Side button): Lower button  
- **Button C** (Thermal2 Module): Button on the thermal sensor
- **PWR Button** (Side button): Power button

### Button Functions

| Action | Function |
|--------|----------|
| **Button A** - Single press | Cycle color scheme (Grayscale → Iron → Rainbow) |
| **Button A** - Hold (Rainbow mode) | Toggle easter egg mode |
| **Button B** - Single press | Adjust brightness (6 levels: 40, 80, 120, 160, 200, 255) |
| **Button B** - Double press | Toggle Night Mode |
| **Button B** - Single press (Night Mode) | Toggle LED flashlight |
| **Button B** - Hold (WiFi connected) | Disable WiFi |
| **Button B** - Hold (WiFi disconnected) | Enable Hotspot Mode |
| **Button C** - Single press | Flip thermal image horizontally |
| **PWR Button** - Single press | Toggle screen blank mode (saves battery) |
| **PWR Button** - Double press | Disable/enable auto power-off |

## Web Interface

When connected to WiFi or in Hotspot Mode, the device runs a web server accessible at:
- **WiFi Mode**: `http://<device-ip>/`
- **Hotspot Mode**: `http://192.168.4.1/` (Default AP IP)
  - SSID: `Thermal`

The web interface provides:
- Live thermal camera feed
- Real-time temperature statistics
- Capture button to save thermal images

## Display Information

The device screen shows:
- **Top Half**: Thermal image with temperature markers
  - Blue box: Coldest point
  - Yellow box: Hottest point
- **Bottom Half**: Status information
  - Battery level and charging status
  - Average, maximum, and minimum temperatures
  - WiFi/Hotspot IP address or version info
  - Auto power-off countdown (when enabled)

## Power Management

- **Auto Power-off**: Device automatically powers off after 2 minutes of inactivity (disabled when charging)
- **Hotspot Timeout**: Hotspot mode disables after 5 minutes of inactivity (disabled when charging)
- **Screen Blank Mode**: Reduces power consumption while maintaining thermal data collection
- **Power-off Warning**: 10-second countdown displayed before shutdown

## Night Mode

Night mode is optimized for use in dark environments:
- Screen brightness set to minimum (1)
- LED flashlight available (white or iron-colored light)
- Only Grayscale and Iron color schemes available
- Ideal for tactical or low-light situations

## Troubleshooting

**Device won't connect to WiFi:**
- Check SSID and password in the source code
- WiFi connection times out after 15 seconds
- Use Hotspot Mode if WiFi is unavailable

**Thermal image appears frozen:**
- Press the PWR button to exit screen blank mode
- Check if the Thermal2 module is properly connected

**Battery drains quickly:**
- Enable auto power-off feature (PWR button double-press)
- Use screen blank mode when not actively viewing
- Disable WiFi when not needed (hold Button B)

**Web interface not accessible:**
- Verify device IP address shown on display
- Ensure device and computer are on the same network
- In Hotspot Mode, connect to "Thermal" WiFi network first

## License

This project is licensed under the MIT License.

### Credits

- Original code: © 2024 M5Stack Technology CO LTD
- Dependencies:
  - [M5Unified](https://github.com/m5stack/M5Unified)
  - [M5Unit-Thermal2](https://github.com/m5stack/M5Unit-Thermal2)

## Version

Current version: 0.7 (Thermal Camera)

