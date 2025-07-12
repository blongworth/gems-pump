/**
 * @file serial_control_example.ino
 * @brief Example of using the ValveManager library with serial control
 */

#include <Arduino.h>

// =============================================================================
// VALVE MANAGER CONFIGURATION
// =============================================================================

// Enable serial control
#define SERIAL_CONTROL

// Override default settings if needed
#define VALVE_SERVO_PIN 1
#define RED_LED_PIN 39
#define GREEN_LED_PIN 36
#define VALVE_CHANGE_INTERVAL 30  // 30 seconds (not used in serial mode)
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
    
    Serial.println("GEMS Pump Control System - Serial Control Example");
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
    Serial.println("Send 't' to move to top, 'b' to move to bottom via Serial2");
}

void loop() {
    // Update the valve manager - handles all valve control and logging
    valveManager.update();
    
    // Optional: Manual control via main serial port
    if (Serial.available()) {
        char command = Serial.read();
        if (command == 't') {
            Serial.println("Manual: Moving to top position");
            valveManager.setValvePosition(VALVE_TOP_POS);
        } else if (command == 'b') {
            Serial.println("Manual: Moving to bottom position");
            valveManager.setValvePosition(VALVE_BOTTOM_POS);
        } else if (command == 'h') {
            Serial.println("Manual: Moving to home position");
            valveManager.setValvePosition(VALVE_HOME_POS);
        }
    }
}
