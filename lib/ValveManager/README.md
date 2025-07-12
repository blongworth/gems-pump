# ValveManager Library

A comprehensive valve management library for the GEMS Pump Control System. This library provides an easy-to-use interface for controlling servo-based valves with integrated power monitoring, data logging, and LED status indicators.

## Features

- **Servo Control**: Precise valve positioning using servo motors
- **Power Monitoring**: Real-time voltage and current measurement using INA260 sensor
- **Data Logging**: Automatic CSV logging to SD card with daily file rotation
- **LED Status**: Visual feedback with configurable LED indicators
- **Serial Communication**: Support for external serial control interfaces
- **Flexible Operation**: Timer-based or serial command-based valve control
- **EEPROM Storage**: Persistent storage of valve position across reboots

## Hardware Requirements

- **Microcontroller**: Teensy 4.1 (or compatible Arduino-based board)
- **Power Sensor**: Adafruit INA260 (I2C)
- **Servo Motor**: Standard servo for valve control
- **SD Card**: Built-in SD card slot for data logging
- **LEDs**: Status indicator LEDs (red, green, heartbeat)
- **RTC**: Real-time clock for timestamping

## Installation

1. Copy the `ValveManager` folder to your Arduino libraries directory
2. Install required dependencies:
   - Adafruit INA260 Library
   - Servo library (built-in)
   - SD library (built-in)
   - Time library
   - EEPROM library (built-in)
   - Flasher library (custom)

## Basic Usage

```cpp
#include <Arduino.h>

// Configure ValveManager before including the library
#define VALVE_SERVO_PIN 1
#define RED_LED_PIN 39
#define GREEN_LED_PIN 36
#define VALVE_CHANGE_INTERVAL 30  // 30 seconds
#define LOG_INTERVAL 10           // Log every 10 seconds
#define VALVE_EXTERNAL_SERIAL Serial2

#include <ValveManager.h>

ValveManager valveManager;

void setup() {
    Serial.begin(115200);
    
    // Initialize - configuration is handled by #define statements above
    if (!valveManager.begin()) {
        Serial.println("Failed to initialize!");
        while(1);
    }
}

void loop() {
    valveManager.update();
}
```

## Configuration Options

### Available Configuration Defines

Override these defines in your sketch before including the ValveManager library:

```cpp
// Pin definitions
#define VALVE_SERVO_PIN 1     // Servo control pin
#define RED_LED_PIN 39        // Red LED pin  
#define GREEN_LED_PIN 36      // Green LED pin

// Timing constants
#define VALVE_CHANGE_INTERVAL 30    // Timer interval (seconds)
#define LOG_INTERVAL 10             // Logging interval (seconds)
#define MIN_MOVE_INTERVAL 2000      // Min time between moves (ms)

// Serial interface
#define VALVE_EXTERNAL_SERIAL Serial2  // External serial interface

// Control mode
#define SERIAL_CONTROL              // Define to enable serial control
```

## API Reference

### Public Methods

#### `ValveManager()`
Constructor that initializes the valve manager. Configuration is handled through preprocessor defines.

#### `bool begin()`
Initializes all system components. Returns `true` if successful, `false` otherwise.

#### `void update()`
Main update function that should be called regularly in the main loop. Handles valve control, logging, and LED updates.

#### `void setValvePosition(int position)`
Manually set the valve to a specific position. Use predefined constants:
- `VALVE_BOTTOM_POS` (1205 µs)
- `VALVE_TOP_POS` (1795 µs)
- `VALVE_HOME_POS` (1500 µs)

#### `int getCurrentPosition()`
Returns the current valve position in microseconds.

#### `int getVoltage()`
Returns the current voltage reading in millivolts.

#### `int getCurrent()`
Returns the current current reading in milliamps.

#### `void logData()`
Forces an immediate data log entry.

#### `bool isInitialized()`
Returns `true` if the valve manager is properly initialized.

## Operation Modes

### Timer-Based Control
Leave `SERIAL_CONTROL` undefined to enable automatic valve cycling based on the configured time interval.

### Serial Control
Define `SERIAL_CONTROL` in your sketch to enable command-based control:
- Send `'t'` to move valve to top position
- Send `'b'` to move valve to bottom position

Commands are received via the serial interface defined by `VALVE_EXTERNAL_SERIAL`.

## Data Logging

The library automatically creates daily CSV log files with the format:
```
gems_pump_YYYY-MM-DD.csv
```

Each log entry contains:
- Timestamp (ISO 8601 format)
- Voltage (mV)
- Current (mA)
- Valve position (microseconds)

## LED Status Indicators

- **Red LED**: Indicates valve at bottom position
- **Green LED**: Indicates valve at top position
- **Both LEDs**: Blinking indicates home position
- **Built-in LED**: Heartbeat indicator

## Examples

The library includes several examples:

1. **basic_valve_control**: Simple timer-based valve control
2. **serial_control_example**: Serial command-based control

## Dependencies

- [Adafruit INA260 Library](https://github.com/adafruit/Adafruit_INA260)
- [Flasher Library](https://github.com/blongworth/flasher-library)
- Arduino built-in libraries: Servo, SD, Time, EEPROM

## License

This library is released under the MIT License.

## Author

Brett Longworth, 2025

Based on original code by Samuel Koeck.
