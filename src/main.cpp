/**
 * @brief GEMS Pump Control System
 * 
 * This program controls a valve system for the GEMS pump, monitoring power conditions
 * and logging operational data. The system includes:
 * 
 * - Power monitoring using INA260 sensor for voltage and current
 * - Servo-controlled valve that can move between low (0°) and high (179°) positions
 * - SD card logging of power data with daily log files
 * - LED status indicators (red and green)
 * - Position request input and position confirmation output
 * 
 * @note The valve will return to home position if voltage drops below threshold
 * @note Based on orginial code by Samuel Koeck
 * 
 * @author Brett Longworth
 * @date 2025
 */

#include <Arduino.h>
#include <TimeLib.h>
#include <Servo.h>
#include <SD.h>
#include <Adafruit_INA260.h>
#include <Flasher.h>
#include <EEPROM.h>

// =============================================================================
// CONFIGURATION
// =============================================================================

// Enable serial control (comment out for timer-based valve control)
// #define SERIAL_CONTROL

// =============================================================================
// CONSTANTS
// =============================================================================

// Timing constants
const unsigned long VALVE_CHANGE_INTERVAL = 450; // 7.5 minutes in seconds
const unsigned long LOG_INTERVAL = 10;           // Data logging interval in seconds
const unsigned long MIN_MOVE_INTERVAL = 2000;    // Minimum time between valve movements (ms)
const unsigned long POWER_CHECK_INTERVAL = 10;   // Power monitoring interval (ms)

// Servo position constants (microseconds)
const int VALVE_BOTTOM_POS = 1205;  // 0 degrees (bottom position)
const int VALVE_TOP_POS = 1795;     // 179 degrees (top position)  
const int VALVE_HOME_POS = 1500;    // 89 degrees (safe home position)

// Power monitoring
const int LOW_VOLTAGE_THRESHOLD = 10000; // Minimum voltage in mV

// Pin definitions
const int VALVE_SERVO_PIN = 1;
const int RED_LED_PIN = 39;
const int GREEN_LED_PIN = 36;

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

Adafruit_INA260 powerSensor;
Servo valve;
char currentLogFile[32] = {0};

// LED indicators
Flasher redLED(RED_LED_PIN, 0, 1000);
Flasher greenLED(GREEN_LED_PIN, 0, 1000); 
Flasher heartbeatLED(LED_BUILTIN, 100, 900);

#define LANDER_SERIAL Serial2

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

void initializeSystem();
void initializeSD();
void logPowerData();
void handleValveControl();
void setValvePosition(int position);
void updateLogFilename();
void checkPowerAndHome();
void updateLEDStatus(int valvePosition);
int calculateExpectedValvePosition();
time_t getTeensy3Time();


// =============================================================================
// SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time for Serial Monitor to open
  Serial.println("GEMS Pump Control System");
  Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);

  initializeSystem();
  initializeSD();
  
#ifdef SERIAL_CONTROL
  // Initialize valve to last known position
  int lastPosition = EEPROM.read(0) ? VALVE_TOP_POS : VALVE_BOTTOM_POS;
  setValvePosition(lastPosition);
#endif
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  checkPowerAndHome();
  handleValveControl();
  updateLogFilename();
  logPowerData();
  
  // Update LED indicators
  redLED.run();
  greenLED.run();
  heartbeatLED.run();
}

// =============================================================================
// SYSTEM INITIALIZATION
// =============================================================================

void initializeSystem() {
  // Initialize serial communication
  LANDER_SERIAL.begin(115200);
  LANDER_SERIAL.println("Lander Serial Initialized");

  // Initialize RTC
  setSyncProvider(getTeensy3Time);
  Serial.println(timeStatus() != timeSet ? "Unable to sync with RTC" : "RTC has set the system time");

  // Initialize power sensor
  if (!powerSensor.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  
  // Initialize servo and LEDs
  delay(4000); // Allow valve to initialize
  valve.attach(VALVE_SERVO_PIN);
  redLED.begin();
  greenLED.begin();
  heartbeatLED.begin();
}

void initializeSD() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  updateLogFilename();
  File dataFile = SD.open(currentLogFile, FILE_WRITE);
  if (dataFile) {
    char timestamp[25];
    time_t t = now();
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
      year(t), month(t), day(t), hour(t), minute(t), second(t));
    dataFile.printf("Rebooted at %s\n", timestamp);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", currentLogFile);
  }
}

// =============================================================================
// FILE MANAGEMENT
// =============================================================================

void updateLogFilename() {
  static time_t lastDay = 0;
  time_t currentTime = now();

  if (strlen(currentLogFile) == 0 || day(currentTime) != day(lastDay)) {
    sprintf(currentLogFile, "gems_pump_%04d-%02d-%02d.csv", 
            year(currentTime), month(currentTime), day(currentTime));

    // Create file with header if it doesn't exist
    if (!SD.exists(currentLogFile)) {
      File dataFile = SD.open(currentLogFile, FILE_WRITE);
      if (dataFile) {
        dataFile.println("timestamp,voltage,current,valve_position"); 
        dataFile.close();
      }
    }
    lastDay = currentTime;
  }
}

// =============================================================================
// DATA LOGGING
// =============================================================================

void logPowerData() {
  static time_t lastLogTime = 0;
  time_t currentTime = now();
  if (currentTime - lastLogTime < LOG_INTERVAL) return;

  // Read sensor data
  int voltage = powerSensor.readBusVoltage();
  int current = powerSensor.readCurrent();
  int valvePosition = valve.readMicroseconds();

  // Format timestamp
  char timestamp[25];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
           year(currentTime), month(currentTime), day(currentTime), 
           hour(currentTime), minute(currentTime), second(currentTime));

  char pos[7] = "bottom";
  if (valvePosition == VALVE_TOP_POS) {
    strcpy(pos, "top");
  }
  // Log to serial
  Serial.printf("Logged Power at %s - Voltage: %d mV, Current: %d mA, Valve Pos: %s\n",
                timestamp, voltage, current, pos);

  char logLine[100];
  snprintf(logLine, sizeof(logLine), "%s,%d,%d,%d\n", timestamp, voltage, current, valvePosition);

  // Log to SD card
  File dataFile = SD.open(currentLogFile, FILE_WRITE);
  if (dataFile) {
    dataFile.print(logLine);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", currentLogFile);
  }
  
  // Log to Lander Serial
  LANDER_SERIAL.printf("V:%s", logLine);

  lastLogTime = now();
}

// =============================================================================
// VALVE CONTROL
// =============================================================================

void handleValveControl() {
  int currentPosition = valve.readMicroseconds();
  int targetPosition = 0;
  
#ifdef SERIAL_CONTROL
  if (LANDER_SERIAL.available()) {
    char command = LANDER_SERIAL.read();
    if (command == 't') {
      targetPosition = VALVE_TOP_POS;
      Serial.println("Serial: Turning to top");
    } else if (command == 'b') {
      targetPosition = VALVE_BOTTOM_POS;
      Serial.println("Serial: Turning to bottom");
    }
  }
#else
  targetPosition = calculateExpectedValvePosition();
#endif
  
  // Move valve if target position is different from current position
  if (targetPosition != 0 && currentPosition != targetPosition) {
#ifndef SERIAL_CONTROL
    // Print debug message only when actually moving in timer mode
    if (targetPosition == VALVE_TOP_POS) {
      Serial.println("Timer: Turning to top");
    } else {
      Serial.println("Timer: Turning to bottom");
    }
#endif
    setValvePosition(targetPosition);
  }
}

void setValvePosition(int position) {
  static unsigned long lastMoveTime = 0;
  unsigned long currentTime = millis();
  
  // Prevent rapid movements
  if (currentTime - lastMoveTime < MIN_MOVE_INTERVAL) return;
  lastMoveTime = currentTime;

  // Check for low power condition
  if (powerSensor.readBusVoltage() < LOW_VOLTAGE_THRESHOLD) {
    Serial.println("Power too low, returning to home position");
    valve.writeMicroseconds(VALVE_HOME_POS);
    updateLEDStatus(VALVE_HOME_POS);
    return;
  }

  // Move valve to requested position
  valve.writeMicroseconds(position);
  EEPROM.update(0, (position == VALVE_TOP_POS) ? 1 : 0);
  updateLEDStatus(position);
}

void updateLEDStatus(int valvePosition) {
  if (valvePosition == VALVE_HOME_POS) {
    // Blinking pattern for home/error position
    redLED.update(200, 800);
    greenLED.update(200, 800);
  } else if (valvePosition == VALVE_TOP_POS) {
    // Red mostly off, green mostly on for top position
    redLED.update(100, 900);
    greenLED.update(0, 1000);
  } else {
    // Red mostly on, green mostly off for bottom position
    redLED.update(0, 1000);
    greenLED.update(100, 900);
  }
}

// =============================================================================
// POWER MONITORING
// =============================================================================

void checkPowerAndHome() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < POWER_CHECK_INTERVAL) return;
  lastCheck = millis();

  int voltage = powerSensor.readBusVoltage();
  if (voltage < LOW_VOLTAGE_THRESHOLD && valve.readMicroseconds() != VALVE_HOME_POS) {
    Serial.println("Low power detected, moving valve to home position");
    valve.writeMicroseconds(VALVE_HOME_POS);
    updateLEDStatus(VALVE_HOME_POS);
  }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

int calculateExpectedValvePosition() {
  time_t currentTime = now();
  int totalSeconds = minute(currentTime) * 60 + second(currentTime);
  int intervalNumber = totalSeconds / VALVE_CHANGE_INTERVAL;
  
  return (intervalNumber % 2 == 0) ? VALVE_BOTTOM_POS : VALVE_TOP_POS;
}