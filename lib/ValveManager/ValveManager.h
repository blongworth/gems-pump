/**
 * @file ValveManager.h
 * @brief Valve Manager Library for GEMS Pump Control System
 * 
 * This library provides a comprehensive valve management system that handles:
 * - Servo-controlled valve positioning
 * - Power monitoring with INA260 sensor
 * - SD card data logging
 * - LED status indicators
 * - Serial communication for external control
 * 
 * @author Brett Longworth
 * @date 2025
 */

#ifndef VALVE_MANAGER_H
#define VALVE_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>
#include <Servo.h>
#include <SD.h>
#include <Adafruit_INA260.h>
#include <Flasher.h>
#include <EEPROM.h>

// =============================================================================
// CONFIGURATION DEFINES - Override these in your main sketch before including
// =============================================================================

// Valve position constants (microseconds)
#define VALVE_BOTTOM_POS 1205  // 0 degrees (bottom position)
#define VALVE_TOP_POS 1795     // 179 degrees (top position)
#define VALVE_HOME_POS 1500    // 89 degrees (safe home position)

// Pin definitions - Override these in your sketch if needed
#ifndef VALVE_SERVO_PIN
#define VALVE_SERVO_PIN 1
#endif

#ifndef RED_LED_PIN
#define RED_LED_PIN 39
#endif

#ifndef GREEN_LED_PIN
#define GREEN_LED_PIN 36
#endif

// Timing constants - Override these in your sketch if needed
#ifndef VALVE_CHANGE_INTERVAL
#define VALVE_CHANGE_INTERVAL 30  // seconds between valve changes
#endif

#ifndef LOG_INTERVAL
#define LOG_INTERVAL 10  // data logging interval in seconds
#endif

#ifndef MIN_MOVE_INTERVAL
#define MIN_MOVE_INTERVAL 2000  // minimum time between valve movements (ms)
#endif

// Control mode - Define SERIAL_CONTROL in your sketch to enable serial control
#ifdef SERIAL_CONTROL
#define VALVE_SERIAL_CONTROL_ENABLED true
#else
#define VALVE_SERIAL_CONTROL_ENABLED false
#endif

// External serial interface - Override in your sketch if needed
#ifndef VALVE_EXTERNAL_SERIAL
#define VALVE_EXTERNAL_SERIAL Serial2
#endif

/**
 * @brief Main valve management class
 */
class ValveManager {
public:
    /**
     * @brief Constructor
     */
    ValveManager();
    
    /**
     * @brief Initialize the valve manager system
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Main update loop - call this regularly in your main loop
     */
    void update();
    
    /**
     * @brief Set valve to specific position
     * @param position Target position (VALVE_BOTTOM_POS, VALVE_TOP_POS, or VALVE_HOME_POS)
     */
    void setValvePosition(int position);
    
    /**
     * @brief Get current valve position
     * @return Current valve position in microseconds
     */
    int getCurrentPosition() const;
    
    /**
     * @brief Check if valve manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief Get current voltage reading
     * @return Voltage in millivolts
     */
    int getVoltage() const;
    
    /**
     * @brief Get current current reading
     * @return Current in milliamps
     */
    int getCurrent() const;
    
    /**
     * @brief Force a data log entry
     */
    void logData();

private:
    // Hardware components
    Adafruit_INA260 _powerSensor;
    Servo _valve;
    Flasher _redLED;
    Flasher _greenLED;
    Flasher _heartbeatLED;
    
    // State management
    bool _initialized = false;
    char _currentLogFile[32] = {0};
    time_t _lastLogTime = 0;
    unsigned long _lastMoveMillis = 0;
    
    // Static function for time sync provider
    static time_t _getTeensy3Time();
    
    // Private methods
    bool _initializeSystem();
    bool _initializeSD();
    void _handleValveControl();
    void _updateLogFilename();
    void _logPowerData(time_t currentTime);
    void _updateLEDStatus(int valvePosition);
    int _calculateExpectedValvePosition(time_t currentTime);
};

#endif // VALVE_MANAGER_H
