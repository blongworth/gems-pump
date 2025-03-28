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
 * @note Valve position changes are rate-limited to once per second
 * @note Based on orginial code by Samuel Koeck
 * 
 * @author Brett Longworth
 * @date 2024
 */

#include <Arduino.h>
#include <TimeLib.h>
#include <Servo.h>
#include <SD.h>
#include <Adafruit_INA260.h>
#include <Flasher.h>

const unsigned long LOG_INTERVAL = 10; // Logging interval in seconds
const int LOW_MICROSECONDS = 1205;  // 0 degrees
const int HIGH_MICROSECONDS = 1795; // 179 degrees
const int HOME_MICROSECONDS = 1500; // 89 degrees
const int POSITION_CHANGE_DELAY = 1000; // in milliseconds
//Threshold voltage = too low power!!
const int THRESHOLD_VOLTAGE = 10000; //in mV

Adafruit_INA260 power = Adafruit_INA260();
int voltage = 0;
int current = 0;

// Global variable for current filename
char filename[32];

// valve servo settings
Servo valve;

// control and readback pins
const int posRequest = 4; //pin for the position request
const int inPosition = 6; //pin for the in position response
                     
// led setup
Flasher red(39, 0, 1000);
Flasher green(36, 0, 1000);

// function prototypes
void measurePower(); // Measure voltage and current from INA260
void logPower(); // Log the power data to the SD card and serial
void turnValve();
void setValvePosition(int position, int ledState);
void updateFilename();
time_t getTeensy3Time();

void setup() {
  Serial.begin(115200);

  // Initialize RTC
  setSyncProvider(getTeensy3Time);
  Serial.println(timeStatus() != timeSet ? "Unable to sync with RTC" : "RTC has set the system time");

  // Initialize SD card
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD card initialization failed!");
    // while (1);
  }

  // Initialize INA260 sensor
  if (!power.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }

  // Initialize valve and LEDs
  valve.attach(1);
  valve.writeMicroseconds(HOME_MICROSECONDS);
  red.update(100, 900);
  green.update(100, 900);

  // Configure pins
  pinMode(posRequest, INPUT_PULLUP);
  delayMicroseconds(10); // Allow pullup to charge
  pinMode(inPosition, OUTPUT);
}

void loop() {
  updateFilename();
  measurePower();
  logPower();
  turnValve();
  red.run();
  green.run();
}

void updateFilename() {
  static time_t lastDay = 0;
  time_t t = now();

  if (day(t) != day(lastDay)) { // Check if day has changed
    sprintf(filename, "gems_pump_%04d-%02d-%02d.csv", year(t), month(t), day(t));

    if (!SD.exists(filename)) { // Create new file with headers if it doesn't exist
      File dataFile = SD.open(filename, FILE_WRITE);
      if (dataFile) {
        dataFile.println("timestamp,voltage,current");
        dataFile.close();
      }
    }
    lastDay = t;
  }
}

void measurePower() {
  voltage = power.readBusVoltage();
  current = power.readCurrent();
}

void logPower() {
  static unsigned long lastLogTime = 0;

  if ((now() - lastLogTime) < LOG_INTERVAL) return; // Skip if interval not met

  measurePower(); // Ensure updated values before logging

  // Log to serial
  Serial.printf("Logged Power - Voltage: %d mV, Current: %d mA\n", voltage, current);

  // Log to SD card
  char timestamp[25];
  time_t t = now();
  sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(t), month(t), day(t), hour(t), minute(t), second(t));

  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.printf("%s,%d,%d\n", timestamp, voltage, current);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", filename);
  }

  lastLogTime = now();
}

void turnValve() {
  static elapsedMillis valveTimer = 0;

  if (valveTimer < POSITION_CHANGE_DELAY) return; // Enforce rate limit

  if (voltage < THRESHOLD_VOLTAGE) {
    Serial.println("Power too low, returning to home position");
    setValvePosition(HOME_MICROSECONDS, LOW);
    valveTimer = 0;
    return;
  }

  if (digitalRead(posRequest) == HIGH && valve.readMicroseconds() < HIGH_MICROSECONDS - 10) {
    Serial.println("Turning to high");
    setValvePosition(HIGH_MICROSECONDS, HIGH);
    valveTimer = 0;
  } else if (digitalRead(posRequest) == LOW && valve.readMicroseconds() > LOW_MICROSECONDS + 10) {
    Serial.println("Turning to low");
    setValvePosition(LOW_MICROSECONDS, LOW);
    valveTimer = 0;
  }
}

void setValvePosition(int position, int ledState) {
  digitalWrite(inPosition, LOW);
  valve.writeMicroseconds(position);
  delay(100);
  digitalWrite(inPosition, HIGH);

  if (ledState == HIGH) {
    red.update(100, 900);
    green.update(0, 1000);
  } else {
    red.update(0, 1000);
    green.update(100, 900);
  }
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}
