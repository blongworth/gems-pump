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

// Optional timer-based valve control
#define TIMED_VALVE_CHANGE // to enable automatic valve switching based on time

// Clock time valve change interval in seconds
const unsigned long VALVE_CHANGE_INTERVAL = 7.5 * 60; // 7:30 minutes

const unsigned long LOG_INTERVAL = 10; // Logging interval in seconds
const int BOTTOM_MICROSECONDS = 1205;  // 0 degrees
const int TOP_MICROSECONDS = 1795; // 179 degrees
const int HOME_MICROSECONDS = 1500; // 89 degrees
//Threshold voltage = too low power!!
const int THRESHOLD_VOLTAGE = 10000; //in mV

Adafruit_INA260 power;
int voltage = 0, current = 0;
char filename[32] = {0};
Servo valve;

Flasher red(39, 0, 1000), green(36, 0, 1000), heartbeat(LED_BUILTIN, 100, 900);

#define LANDER_SERIAL Serial2

void logPower();
void turnValve();
void setValvePosition(int position);
void updateFilename();
time_t getTeensy3Time();
bool isIntervalTime(int intervalSeconds);
void checkAndHomeOnLowPower();

void setup() {
  Serial.begin(115200);
  Serial.println("GEMS Pump Control System");
  Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);

  LANDER_SERIAL.begin(115200);
  LANDER_SERIAL.println("Lander Serial Initialized");

  setSyncProvider(getTeensy3Time);
  Serial.println(timeStatus() != timeSet ? "Unable to sync with RTC" : "RTC has set the system time");

  if (!SD.begin(BUILTIN_SDCARD)) Serial.println("SD card initialization failed!");

  updateFilename();
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    char timestamp[25];
    time_t t = now();
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
      year(t), month(t), day(t), hour(t), minute(t), second(t));
    dataFile.printf("Rebooted at %s\n", timestamp);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", filename);
  }

  if (!power.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  
  // delay to allow valve to initialize and home
  delay(4000);

  valve.attach(1);
  red.begin();
  green.begin();
  heartbeat.begin();

  int setPos = EEPROM.read(0) ? TOP_MICROSECONDS : BOTTOM_MICROSECONDS;
  setValvePosition(setPos);
}

void loop() {
  checkAndHomeOnLowPower();
  turnValve();
  updateFilename();
  logPower();
  red.run();
  green.run();
  heartbeat.run();
}

void updateFilename() {
  static time_t lastDay = 0;
  time_t t = now();

  if (strlen(filename) == 0 || day(t) != day(lastDay)) { // Check if filename not set or day has changed
    sprintf(filename, "gems_pump_%04d-%02d-%02d.csv", year(t), month(t), day(t));

    if (!SD.exists(filename)) {
      File dataFile = SD.open(filename, FILE_WRITE);
      if (dataFile) {
        dataFile.println("timestamp,voltage,current,valve_position"); 
        dataFile.close();
      }
    }
    lastDay = t;
  }
}

void logPower() {
  static unsigned long lastLogTime = 0;
  if ((now() - lastLogTime) < LOG_INTERVAL) return;

  // Read power and valve position
  voltage = power.readBusVoltage();
  current = power.readCurrent();
  int valve_pos = valve.readMicroseconds();

  // Format timestamp
  char timestamp[25];
  time_t t = now();
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
           year(t), month(t), day(t), hour(t), minute(t), second(t));

  // Log to serial
  Serial.printf("Logged Power at %s - Voltage: %d mV, Current: %d mA, Valve Pos: %d\n",
                timestamp, voltage, current, valve_pos);

  // Log to SD card
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.printf("%s,%d,%d,%d\n", timestamp, voltage, current, valve_pos);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", filename);
  }

  lastLogTime = now();
}

#ifndef TIMED_VALVE_CHANGE
void sendPos(char pos) {
  LANDER_SERIAL.write(pos);
  LANDER_SERIAL.flush();
  Serial.printf("Sent position: %c\n", pos);
} 
#endif

void turnValve() {
#ifdef TIMED_VALVE_CHANGE
  if (isIntervalTime(VALVE_CHANGE_INTERVAL)) {
    if (valve.readMicroseconds() == BOTTOM_MICROSECONDS) {
      Serial.println("Timer: Turning to top");
      setValvePosition(TOP_MICROSECONDS);
    } else {
      Serial.println("Timer: Turning to bottom");
      setValvePosition(BOTTOM_MICROSECONDS);
    }
  }
#else
  if (LANDER_SERIAL.available()) {
    char command = LANDER_SERIAL.read();
    if (command == 't' && valve.readMicroseconds() < TOP_MICROSECONDS - 10) {
      Serial.println("Turning to top");
      setValvePosition(TOP_MICROSECONDS);
    } else if (command == 'b' && valve.readMicroseconds() > BOTTOM_MICROSECONDS + 10) {
      Serial.println("Turning to bottom");
      setValvePosition(BOTTOM_MICROSECONDS);
    }
  }
#endif
}

void setValvePosition(int position) {

  // Servo will lose it's home position if power is too low
  // Setting to home gives us a chance it will be OK when power returns
  if (power.readBusVoltage() < THRESHOLD_VOLTAGE) {
    Serial.println("Power too low, returning to home position");
    valve.writeMicroseconds(HOME_MICROSECONDS);
    red.update(200, 800);
    green.update(200, 800);
    return;
  }

  EEPROM.update(0, (position == TOP_MICROSECONDS) ? 1 : 0); // Store position in EEPROM

  #ifndef TIMED_VALVE_CHANGE
  char posChar = (position == BOTTOM_MICROSECONDS) ? 'b' : 't';
  sendPos(posChar);
  #endif

  if (position == TOP_MICROSECONDS) {
    red.update(100, 900);
    green.update(0, 1000);
  } else {
    red.update(0, 1000);
    green.update(100, 900);
  }
}

void checkAndHomeOnLowPower() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 10) return;
  lastCheck = millis();

  voltage = power.readBusVoltage();
  if (voltage < THRESHOLD_VOLTAGE && valve.readMicroseconds() != HOME_MICROSECONDS) {
    Serial.println("Low power detected, moving valve to home position");
    valve.writeMicroseconds(HOME_MICROSECONDS);
    red.update(200, 800);
    green.update(200, 800);
  }
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

bool isIntervalTime(int intervalSeconds) {
  time_t t = now();
  int minutes = minute(t);
  int seconds = second(t);
  int totalSeconds = minutes * 60 + seconds;
  return (totalSeconds % intervalSeconds) == 0;
}