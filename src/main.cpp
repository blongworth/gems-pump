/**
 * @brief GEMS Pump Control System
 * 
 * This program controls a valve system for the GEMS pump using the ValveManager library.
 * The system monitors power conditions and logs operational data.
 * 
 * Features:
 * - Power monitoring using INA260 sensor for voltage and current
 * - Servo-controlled valve that can move between low (0°) and high (179°) positions
 * - SD card logging of power data with daily log files
 * - LED status indicators (red and green)
 * - Position request input and position confirmation output
 * - Serial control or timer-based operation
 * 
 * @note The valve will return to home position if voltage drops below threshold
 * @note Based on original code by Samuel Koeck
 * 
 * @author Brett Longworth
 * @date 2025
 */

#include <Arduino.h>

// =============================================================================
// VALVE MANAGER CONFIGURATION
// =============================================================================

// Enable serial control (comment out for timer-based valve control)
// #define SERIAL_CONTROL

// Override default pin assignments if needed
#define VALVE_SERVO_PIN 1
#define RED_LED_PIN 39
#define GREEN_LED_PIN 36

// Override default timing if needed
#define VALVE_CHANGE_INTERVAL 30  // 30 seconds between valve changes
#define LOG_INTERVAL 10           // Log data every 10 seconds
#define MIN_MOVE_INTERVAL 2000    // Minimum 2 seconds between moves

// Define external serial interface
#define VALVE_EXTERNAL_SERIAL Serial2

// Include ValveManager after configuration
#include <ValveManager.h>

ValveManager valveManager;


// =============================================================================
// SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for Serial Monitor to open
  Serial.println("GEMS Pump Control System");
  Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);

  // Initialize the valve manager
  // Configuration is handled by #define statements above
  if (!valveManager.begin()) {
    Serial.println("Failed to initialize ValveManager!");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("System initialization complete");
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  // Update the valve manager - handles all valve control and logging
  valveManager.update();
}

