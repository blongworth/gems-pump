/**
 * @file custom_config_example.ino
 * @brief Example showing how to customize ValveManager configuration
 */

#include <Arduino.h>

// =============================================================================
// VALVE MANAGER CONFIGURATION
// =============================================================================

// Custom pin assignments
#define VALVE_SERVO_PIN 5         // Use pin 5 for servo instead of default 1
#define RED_LED_PIN 10            // Use pin 10 for red LED instead of default 39
#define GREEN_LED_PIN 11          // Use pin 11 for green LED instead of default 36

// Custom timing settings
#define VALVE_CHANGE_INTERVAL 60  // 60 seconds between valve changes (instead of 30)
#define LOG_INTERVAL 5            // Log every 5 seconds (instead of 10)
#define MIN_MOVE_INTERVAL 1000    // 1 second minimum between moves (instead of 2)

// Enable serial control for this example
#define SERIAL_CONTROL

// Use Serial1 instead of Serial2 for external communication
#define VALVE_EXTERNAL_SERIAL Serial1

// Include ValveManager after all configuration
#include <ValveManager.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

ValveManager valveManager;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("GEMS Pump Control System - Custom Configuration Example");
    Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);
    
    // Print configuration being used
    Serial.println("\nConfiguration:");
    Serial.printf("  Servo Pin: %d\n", VALVE_SERVO_PIN);
    Serial.printf("  Red LED Pin: %d\n", RED_LED_PIN);
    Serial.printf("  Green LED Pin: %d\n", GREEN_LED_PIN);
    Serial.printf("  Valve Change Interval: %d seconds\n", VALVE_CHANGE_INTERVAL);
    Serial.printf("  Log Interval: %d seconds\n", LOG_INTERVAL);
    Serial.printf("  Min Move Interval: %d ms\n", MIN_MOVE_INTERVAL);
    Serial.printf("  Serial Control: %s\n", VALVE_SERIAL_CONTROL_ENABLED ? "Enabled" : "Disabled");
    
    // Initialize the valve manager
    if (!valveManager.begin()) {
        Serial.println("Failed to initialize ValveManager!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("ValveManager initialized successfully");
    Serial.println("Send 't' for top, 'b' for bottom, 'h' for home via Serial or Serial1");
}

void loop() {
    // Update the valve manager
    valveManager.update();
    
    // Manual control via main serial port
    if (Serial.available()) {
        char command = Serial.read();
        handleCommand(command, "Serial");
    }
    
    // Print status every 10 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        Serial.printf("Status - Position: %d, Voltage: %d mV, Current: %d mA\n",
                      valveManager.getCurrentPosition(),
                      valveManager.getVoltage(),
                      valveManager.getCurrent());
        lastStatus = millis();
    }
}

void handleCommand(char command, const char* source) {
    switch (command) {
        case 't':
            Serial.printf("%s: Moving to top position\n", source);
            valveManager.setValvePosition(VALVE_TOP_POS);
            break;
        case 'b':
            Serial.printf("%s: Moving to bottom position\n", source);
            valveManager.setValvePosition(VALVE_BOTTOM_POS);
            break;
        case 'h':
            Serial.printf("%s: Moving to home position\n", source);
            valveManager.setValvePosition(VALVE_HOME_POS);
            break;
        case 'l':
            Serial.printf("%s: Forcing data log\n", source);
            valveManager.logData();
            break;
        case 's':
            Serial.printf("%s: Status - Position: %d, Voltage: %d mV, Current: %d mA\n",
                          source,
                          valveManager.getCurrentPosition(),
                          valveManager.getVoltage(),
                          valveManager.getCurrent());
            break;
        default:
            Serial.printf("%s: Unknown command '%c'. Use: t(top), b(bottom), h(home), l(log), s(status)\n", 
                          source, command);
            break;
    }
}
