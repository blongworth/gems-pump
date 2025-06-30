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
const int LOW_MICROSECONDS = 1205;  // 0 degrees
const int HIGH_MICROSECONDS = 1795; // 179 degrees
const int HOME_MICROSECONDS = 1500; // 89 degrees
const int POSITION_CHANGE_DELAY = 2000; // in milliseconds
//Threshold voltage = too low power!!
const int THRESHOLD_VOLTAGE = 10000; //in mV

Adafruit_INA260 power = Adafruit_INA260();
int voltage = 0;
int current = 0;

// Global variable for current filename
char filename[32] = {0};

// valve servo settings
Servo valve;

// led setup
Flasher red(39, 0, 1000);
Flasher green(36, 0, 1000);
Flasher heartbeat(LED_BUILTIN, 100, 900); // Heartbeat LED on built-in LED pin

// Lander Serial
#define LANDER_SERIAL Serial2

// function prototypes
void measurePower(); // Measure voltage and current from INA260
void logPower(); // Log the power data to the SD card and serial
void turnValve();
void setValvePosition(int position, int ledState);
void updateFilename();
time_t getTeensy3Time();
bool isIntervalTime(int intervalSeconds);

void setup() {
  Serial.begin(115200);
  Serial.println("GEMS Pump Control System");
  Serial.printf("Compiled: %s %s\n", __DATE__, __TIME__);

  LANDER_SERIAL.begin(115200);
  LANDER_SERIAL.println("Lander Serial Initialized");


  // Initialize RTC
  setSyncProvider(getTeensy3Time);
  Serial.println(timeStatus() != timeSet ? "Unable to sync with RTC" : "RTC has set the system time");

  // Initialize SD card
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD card initialization failed!");
  }

  // log reboots
  updateFilename();
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    char timestamp[25];
    time_t t = now();
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(t), month(t), day(t), hour(t), minute(t), second(t));
    dataFile.printf("Rebooted at %s\n", timestamp);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", filename);
  }

  // Initialize INA260 sensor
  if (!power.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }

  // Initialize valve and LEDs
  valve.attach(1);

  red.begin();
  green.begin();
  heartbeat.begin();

  // Set initial valve position to last position from EEPROM
  int lastPosition = EEPROM.read(0); // Read last position from EEPROM

  int setPos = (lastPosition) ? HIGH_MICROSECONDS : LOW_MICROSECONDS;
  setValvePosition(setPos, lastPosition ? HIGH : LOW);

}

void loop() {
  updateFilename();
  measurePower();
  logPower();
  turnValve();
  red.run();
  green.run();
  heartbeat.run();
}

void updateFilename() {
  static time_t lastDay = 0;
  time_t t = now();

  if (strlen(filename) == 0 || day(t) != day(lastDay)) { // Check if filename not set or day has changed
    sprintf(filename, "gems_pump_%04d-%02d-%02d.csv", year(t), month(t), day(t));

    if (!SD.exists(filename)) { // Create new file with headers if it doesn't exist
      File dataFile = SD.open(filename, FILE_WRITE);
      if (dataFile) {
        dataFile.println("timestamp,voltage,current,valve_position"); 
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
  int valve_pos = valve.readMicroseconds(); // Read current valve position in microseconds

  // Log to SD card
  char timestamp[25];
  time_t t = now();
  sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ", year(t), month(t), day(t), hour(t), minute(t), second(t));
  
  // Log to serial
  Serial.printf("Logged Power at %s - Voltage: %d mV, Current: %d mA, Valve Pos: %d\n",
    timestamp, voltage, current, valve_pos);


  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.printf("%s,%d,%d,%d\n", timestamp, voltage, current, valve_pos);
    dataFile.close();
  } else {
    Serial.printf("Error opening %s\n", filename);
  }

  lastLogTime = now();
}

void sendPos(char pos) {
  LANDER_SERIAL.write(pos);
  LANDER_SERIAL.flush();
  Serial.printf("Sent position: %c\n", pos);
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

#ifdef TIMED_VALVE_CHANGE
  if (isIntervalTime(VALVE_CHANGE_INTERVAL)) {
    if (valve.readMicroseconds() == LOW_MICROSECONDS) {
      Serial.println("Timer: Turning to top");
      setValvePosition(HIGH_MICROSECONDS, HIGH);
    } else {
      Serial.println("Timer: Turning to bottom");
      setValvePosition(LOW_MICROSECONDS, LOW);
    }
    valveTimer = 0;
  }
#else
  if (LANDER_SERIAL.available()) {
    char command = LANDER_SERIAL.read();
    if (command == 't' && valve.readMicroseconds() < HIGH_MICROSECONDS - 10) {
      Serial.println("Turning to high");
      setValvePosition(HIGH_MICROSECONDS, HIGH);
    } else if (command == 'b' && valve.readMicroseconds() > LOW_MICROSECONDS + 10) {
      Serial.println("Turning to low");
      setValvePosition(LOW_MICROSECONDS, LOW);
    }
  }
#endif
}

void setValvePosition(int position, int ledState) {
  valve.writeMicroseconds(position);
  EEPROM.update(0, (position == HIGH_MICROSECONDS) ? 1 : 0); // Store position in EEPROM
  char posChar = (position == LOW_MICROSECONDS) ? 'b' : 't';
  sendPos(posChar);
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

bool isIntervalTime(int intervalSeconds) {
  time_t t = now();
  int minutes = minute(t);
  int seconds = second(t);
  int totalSeconds = minutes * 60 + seconds;
  return (totalSeconds % intervalSeconds) == 0;
}