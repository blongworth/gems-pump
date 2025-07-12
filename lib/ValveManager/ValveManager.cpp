/**
 * @file ValveManager.cpp
 * @brief Implementation of ValveManager class
 */

#include "ValveManager.h"

ValveManager::ValveManager() 
    : _redLED(RED_LED_PIN, 0, 1000),
      _greenLED(GREEN_LED_PIN, 0, 1000),
      _heartbeatLED(LED_BUILTIN, 100, 900) {
}

bool ValveManager::begin() {
    Serial.println("Initializing ValveManager...");
    
    if (!_initializeSystem()) {
        Serial.println("Failed to initialize system components");
        return false;
    }
    
    if (!_initializeSD()) {
        Serial.println("Failed to initialize SD card");
        return false;
    }
    
    // Initialize valve to last known position if serial control is enabled
    if (VALVE_SERIAL_CONTROL_ENABLED) {
        int lastPosition = EEPROM.read(0) ? VALVE_TOP_POS : VALVE_BOTTOM_POS;
        setValvePosition(lastPosition);
    }
    
    _initialized = true;
    Serial.println("ValveManager initialized successfully");
    return true;
}

void ValveManager::update() {
    if (!_initialized) return;
    
    _handleValveControl();
    _updateLogFilename();
    
    // Update LED indicators
    _redLED.run();
    _greenLED.run();
    _heartbeatLED.run();
}

void ValveManager::setValvePosition(int position) {
    if (!_initialized) return;
    
    unsigned long currentMillis = millis();
    
    // Prevent rapid movements
    if (currentMillis - _lastMoveMillis < MIN_MOVE_INTERVAL) return;
    _lastMoveMillis = currentMillis;
    
    // Move valve to requested position
    _valve.writeMicroseconds(position);
    EEPROM.update(0, (position == VALVE_TOP_POS) ? 1 : 0);
    _updateLEDStatus(position);
    
    Serial.printf("Valve moved to position: %d microseconds\n", position);
}

int ValveManager::getCurrentPosition() const {
    if (!_initialized) return 0;
    // Note: readMicroseconds() is not const, but we need to call it anyway
    return const_cast<Servo&>(_valve).readMicroseconds();
}

int ValveManager::getVoltage() const {
    if (!_initialized) return 0;
    // Note: readBusVoltage() is not const, but we need to call it anyway
    return const_cast<Adafruit_INA260&>(_powerSensor).readBusVoltage();
}

int ValveManager::getCurrent() const {
    if (!_initialized) return 0;
    // Note: readCurrent() is not const, but we need to call it anyway
    return const_cast<Adafruit_INA260&>(_powerSensor).readCurrent();
}

void ValveManager::logData() {
    if (!_initialized) return;
    _logPowerData(now());
}

bool ValveManager::_initializeSystem() {
    // Initialize external serial communication
    VALVE_EXTERNAL_SERIAL.begin(115200);
    VALVE_EXTERNAL_SERIAL.println("External Serial Initialized");
    
    // Initialize RTC
    setSyncProvider(_getTeensy3Time);
    if (timeStatus() != timeSet) {
        Serial.println("Unable to sync with RTC");
        return false;
    }
    Serial.println("RTC has set the system time");
    
    // Initialize power sensor
    if (!_powerSensor.begin()) {
        Serial.println("Couldn't find INA260 chip");
        return false;
    }
    Serial.println("INA260 power sensor initialized");
    
    // Initialize servo and LEDs
    delay(4000); // Allow valve to initialize
    _valve.attach(VALVE_SERVO_PIN);
    _redLED.begin();
    _greenLED.begin();
    _heartbeatLED.begin();
    
    Serial.println("Servo and LEDs initialized");
    return true;
}

bool ValveManager::_initializeSD() {
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD card initialization failed!");
        return false;
    }
    
    _updateLogFilename();
    File dataFile = SD.open(_currentLogFile, FILE_WRITE);
    if (dataFile) {
        char timestamp[25];
        time_t t = now();
        sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                year(t), month(t), day(t), hour(t), minute(t), second(t));
        dataFile.printf("Rebooted at %s\n", timestamp);
        dataFile.close();
        Serial.printf("SD card initialized, log file: %s\n", _currentLogFile);
    } else {
        Serial.printf("Error opening %s\n", _currentLogFile);
        return false;
    }
    
    return true;
}

void ValveManager::_handleValveControl() {
    time_t currentTime = now();
    int currentPosition = _valve.readMicroseconds();
    int targetPosition = 0;
    
    if (VALVE_SERIAL_CONTROL_ENABLED) {
        if (VALVE_EXTERNAL_SERIAL.available()) {
            char command = VALVE_EXTERNAL_SERIAL.read();
            if (command == 't') {
                targetPosition = VALVE_TOP_POS;
                Serial.println("Serial: Turning to top");
            } else if (command == 'b') {
                targetPosition = VALVE_BOTTOM_POS;
                Serial.println("Serial: Turning to bottom");
            }
        }
    } else {
        targetPosition = _calculateExpectedValvePosition(currentTime);
    }
    
    // Move valve if target position is different from current position
    if (targetPosition != 0 && currentPosition != targetPosition) {
        if (!VALVE_SERIAL_CONTROL_ENABLED) {
            // Print debug message only when actually moving in timer mode
            if (targetPosition == VALVE_TOP_POS) {
                Serial.println("Timer: Turning to top");
            } else {
                Serial.println("Timer: Turning to bottom");
            }
        }
        setValvePosition(targetPosition);
    }
    
    // Handle logging after valve control to log correct position
    if (currentTime <= _lastLogTime) return;
    if (second(currentTime) % LOG_INTERVAL != 0) return;
    _logPowerData(currentTime);
    _lastLogTime = now();
}

void ValveManager::_updateLogFilename() {
    static time_t lastDay = 0;
    time_t currentTime = now();
    
    if (strlen(_currentLogFile) == 0 || day(currentTime) != day(lastDay)) {
        sprintf(_currentLogFile, "gems_pump_%04d-%02d-%02d.csv",
                year(currentTime), month(currentTime), day(currentTime));
        
        // Create file with header if it doesn't exist
        if (!SD.exists(_currentLogFile)) {
            File dataFile = SD.open(_currentLogFile, FILE_WRITE);
            if (dataFile) {
                dataFile.println("timestamp,voltage,current,valve_position");
                dataFile.close();
            }
        }
        lastDay = currentTime;
    }
}

void ValveManager::_logPowerData(time_t currentTime) {
    // Read sensor data
    int voltage = _powerSensor.readBusVoltage();
    int current = _powerSensor.readCurrent();
    int valvePosition = _valve.readMicroseconds();
    
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
    char logSerial[100];
    snprintf(logSerial, sizeof(logSerial),
             "Logged Power at %s - Voltage: %d mV, Current: %d mA, Valve Pos: %s\n",
             timestamp, voltage, current, pos);
    Serial.print(logSerial);
    
    char logLine[100];
    snprintf(logLine, sizeof(logLine), "%s,%d,%d,%d\n", timestamp, voltage, current, valvePosition);
    
    // Log to SD card
    File dataFile = SD.open(_currentLogFile, FILE_WRITE);
    if (dataFile) {
        dataFile.print(logLine);
        dataFile.close();
    } else {
        Serial.printf("Error opening %s\n", _currentLogFile);
    }
    
    // Log to external serial
    VALVE_EXTERNAL_SERIAL.printf("V:%s", logLine);
}

void ValveManager::_updateLEDStatus(int valvePosition) {
    if (valvePosition == VALVE_HOME_POS) {
        _redLED.update(200, 800);
        _greenLED.update(200, 800);
    } else if (valvePosition == VALVE_TOP_POS) {
        _redLED.update(100, 900);
        _greenLED.update(0, 1000);
    } else {
        _redLED.update(0, 1000);
        _greenLED.update(100, 900);
    }
}

// Static function for time sync provider
time_t ValveManager::_getTeensy3Time() {
    return Teensy3Clock.get();
}

int ValveManager::_calculateExpectedValvePosition(time_t currentTime) {
    int totalSeconds = minute(currentTime) * 60 + second(currentTime);
    int intervalNumber = totalSeconds / VALVE_CHANGE_INTERVAL;
    return (intervalNumber % 2 == 0) ? VALVE_BOTTOM_POS : VALVE_TOP_POS;
}
