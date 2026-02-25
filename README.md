# ATSU - Affordable Thermal Sensing Unit

A feature-rich thermal imaging camera firmware for the M5StickC Plus2 with M5Stack Thermal2 module.

## Features

- **Real-time Thermal Imaging**: 32x24 thermal sensor with adaptive refresh rate (2-32Hz)
- **Multiple Color Schemes**: 
  - Grayscale
  - Iron (black → red → yellow → white)
  - Rainbow (blue → cyan → green → yellow → red)
- **Night Mode**: Low brightness with LED flashlight functionality (white or iron-colored)
- **Insomnia Mode**: Disable auto power-off to keep device running indefinitely
- **Low Power Mode**: Automatically activates at ≤20% battery to extend runtime
- **Dowsing Mode**: LED thermal indicators when screen is blank for hands-free operation
- **WiFi Connectivity**: Connect to your network or create a hotspot
- **Web Interface**: View thermal camera feed remotely via browser
- **Auto Power-off**: Configurable automatic shutdown after 2 minutes of inactivity
- **LED Indicators**: Visual feedback via thermal module and device LEDs with temperature-based colors
- **Orientation Detection**: Auto-rotating text based on device orientation
- **Temperature Statistics**: Real-time display of average, max, and min temperatures
- **Audio Feedback**: Boot chimes, power-off alerts, and interaction sounds
- **Battery Management**: Smart averaging and visual indicators with charging detection

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
   - Create a file named `WiFiSecrets.h` in the same directory as `atsu.ino` with your WiFi credentials:
     ```cpp
     #define WIFI_SSID "YourSSID"
     #define WIFI_PASS "YourPassword"
     ```
   - If you don't create this file, WiFi will be disabled by default

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
| **Button B** - Single press | Adjust brightness (6 levels: 40, 80, 120, 160, 200, 255) |
| **Button B** - Single press (Night Mode) | Toggle LED flashlight |
| **Button B** - Double press | Toggle Night Mode |
| **Button B** - Hold (WiFi connected) | Disable WiFi |
| **Button B** - Hold (WiFi disconnected) | Enable Hotspot Mode |
| **Button B** - Hold (Dowsing Mode) | Toggle audio mode (continuous tone ↔ beeping) |
| **Button C** - Single press | Flip thermal image horizontally |
| **PWR Button** - Single press | Toggle screen blank mode (saves battery, enables Dowsing mode) |
| **PWR Button** - Double press | Toggle Insomnia Mode (disable/enable auto power-off) |

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
  - Battery level (color-coded) and charging status
  - Average, maximum, and minimum temperatures
  - WiFi/Hotspot IP address or version info
  - Special mode indicators:
    - "NIGHT MODE" in purple (when active)
    - "INSOMNIA MODE" in red (when auto power-off disabled)
    - Power-off countdown warning (last 10 seconds)

## Audio Feedback

The device provides audio cues for various events (when speaker is enabled):
- **Boot Chime**: Two-tone ascending chime on startup
- **Power-off Alert**: Three-tone descending chime before shutdown
- **Screen Blank Beep**: Short beep when toggling screen blank mode
- All sounds use appropriate volume levels for different contexts
    
## Power Management

- **Auto Poweroff**: Device automatically powers off after 2 minutes of inactivity (disabled when charging)
  - Visual countdown warning appears in the last 10 seconds
  - Audio alert plays before shutdown
- **Insomnia Mode**: Prevents auto power-off (indicated by "INSOMNIA MODE" text on display)
  - Toggle with PWR button double-press
  - Useful for continuous monitoring or web streaming
- **Low Power Mode**: Automatically activates when battery drops to ≤20%
  - Forces minimum brightness (1)
  - Disables WiFi, night mode, and LEDs
  - Reduces refresh rate to extend battery life
- **Hotspot Timeout**: Hotspot mode automatically disables after 5 minutes of inactivity (disabled when charging)
- **Screen Blank Mode**: Reduces power consumption while maintaining thermal data collection
  - Enables Dowsing Mode with LED thermal indicators
  - Adaptive refresh rate (2Hz or 4Hz) based on mode
- **Battery Monitoring**: Rolling average of battery percentage with visual indicators
  - Green (≥80%), Yellow (≥60%), Orange (≥40%), Red (≥20%), Dark Red (<20%)
  - Shows charging status with "+" indicator
  
## Night Mode

Night mode is optimized for use in dark environments:
- Screen brightness set to minimum (1)
- LED flashlight available (white or iron-colored light based on color scheme)
- Only Grayscale and Iron color schemes available
- Ideal for tactical or low-light situations
- Thermal module and device LEDs illuminate with selected color

## Dowsing Mode

Activated automatically when screen blank mode is enabled (without Night Mode):
- LEDs display temperature-based colors for hands-free thermal detection
- Both thermal module LED and device LED show thermal indicators
- Color changes based on average temperature and contrast
- Optimized 4Hz refresh rate for battery efficiency
- Useful for finding heat sources without looking at the screen
- **Audio Feedback**: Provides temperature-based audio tones
  - **Continuous Tone Mode** (default): Steady tone that changes pitch based on temperature
  - **Beeping Mode**: Intermittent beeps with frequency varying by temperature contrast
  - Toggle between modes by holding Button B while in Dowsing Mode
  - Pitch and interval dynamically adjust to thermal readings for intuitive feedback

## Insomnia Mode

Prevents automatic power-off for continuous operation:
- Toggle with PWR button double-press
- Indicated by "INSOMNIA MODE" text on display (in red)
- Useful for extended monitoring sessions or when using web interface
- Device will still respond to manual power-off commands
- Automatically disabled in Low Power Mode

## Low Power Mode

Automatically activates when battery level drops to 20% or below:
- Cannot be manually disabled - only deactivates when battery is charged above 20%
- Forces minimum screen brightness (1)
- Disables WiFi and hotspot functionality
- Disables Night Mode and Insomnia Mode
- Turns off all LED indicators (thermal module and device LEDs)
- Reduces thermal sensor refresh rate to conserve battery
- Designed to maximize remaining runtime in critical battery situations

## Troubleshooting

**Device won't connect to WiFi:**
- Check SSID and password in the source code
- WiFi connection times out after 15 seconds
- Use Hotspot Mode if WiFi is unavailable

**Thermal image appears frozen:**
- Press the PWR button to exit screen blank mode
- Check if the Thermal2 module is properly connected

**Battery drains quickly:**
- Ensure Insomnia Mode is disabled (PWR button double-press should show no "INSOMNIA MODE" text)
- Use screen blank mode when not actively viewing (enables Dowsing mode for LED indicators)
- Disable WiFi when not needed (hold Button B)
- Device automatically enters Low Power Mode at ≤20% battery

**Web interface not accessible:**
- Verify device IP address shown on display
- Ensure device and computer are on the same network
- In Hotspot Mode, connect to "Thermal" WiFi network first
- WiFi connection times out after 15 seconds if network is unavailable
- Ensure WiFiSecrets.h file exists with correct credentials

**Low Power Mode activates unexpectedly:**
- Low Power Mode automatically engages when battery drops to ≤20%
- Charge the device above 20% to restore full functionality
- This is an automatic battery protection feature and cannot be manually disabled

## License

This project is licensed under the MIT License.

### Credits

- Original code: © 2024 M5Stack Technology CO LTD
- Dependencies:
  - [M5Unified](https://github.com/m5stack/M5Unified)
  - [M5Unit-Thermal2](https://github.com/m5stack/M5Unit-Thermal2)

## Version

Current version: 0.7 (Thermal Camera)

