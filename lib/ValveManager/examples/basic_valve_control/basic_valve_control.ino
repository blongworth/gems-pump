/**
 * @file basic_valve_control.ino
 * @brief Basic example of using the ValveManager library
 */

#include <Arduino.h>

// =============================================================================
// VALVE MANAGER CONFIGURATION
// =============================================================================

// Timer-based control (no serial control)
// #define SERIAL_CONTROL  // Leave commented for timer-based control

// Override default settings if needed
#define VALVE_SERVO_PIN 1
#define RED_LED_PIN 39
#define GREEN_LED_PIN 36
#define VALVE_CHANGE_INTERVAL 30  // 30 seconds between valve changes
#define LOG_INTERVAL 10           // Log data every 10 seconds
#define MIN_MOVE_INTERVAL 2000    // Minimum 2 seconds between moves

// Define external serial interface
#define VALVE_EXTERNAL_SERIAL Serial2

// Include ValveManager after configuration
#include <ValveManager.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

ValveManager valveManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("GEMS Pump Control System - Basic Example");
    Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);
    
    // Initialize the valve manager
    // Configuration is handled by #define statements above
    if (!valveManager.begin()) {
        Serial.println("Failed to initialize ValveManager!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("ValveManager initialized successfully");
}

void loop() {
    // Update the valve manager - handles all valve control and logging
    valveManager.update();
    
    // Optional: Print status every 5 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        Serial.printf("Current Position: %d, Voltage: %d mV, Current: %d mA\n",
                      valveManager.getCurrentPosition(),
                      valveManager.getVoltage(),
                      valveManager.getCurrent());
        lastStatus = millis();
    }
}
