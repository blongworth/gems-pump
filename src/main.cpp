#include <Arduino.h>
#include <TimeLib.h>
#include <Servo.h>
#include <Adafruit_INA260.h>
#include <Flasher.h>

Adafruit_INA260 power = Adafruit_INA260();
//Threshold voltage = too low power!!
const int thresholdVoltage = 10000; //in mV

// valve servo settings
Servo valve;
const int low=1205; //0 degrees 1205 us
const int high=1795; //179 degrees 1795 us
const int home=1500; //89 degrees 1500 us

// control and readback pins
const int posRequest = 4; //pin for the position request
const int inPosition = 6; //pin for the in position response
                     
// led setup
Flasher red(39, 0, 1000);
Flasher green(36, 0, 1000);

// put function declarations here:
void turnValve();

void setup() {
  Serial.begin(115200);
  if (!power.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  valve.attach(1);
  valve.writeMicroseconds(home);
  red.update(100, 900);
  green.update(100, 900);
  pinMode(posRequest, INPUT_PULLUP); // state will be HIGH if disconnected
  delayMicroseconds(10); // allow pullup to charge
  pinMode(inPosition, OUTPUT);
}

void loop() {
  turnValve();
  red.run();
  green.run();
}

void turnValve() {
  static elapsedMillis valveTimer = 0;
  if (valveTimer < 1000) {
    return; // don't change valve more often than every second
  }
  if (power.readBusVoltage() < thresholdVoltage) {
    Serial.println("Power too low, returning to home position");
    valve.writeMicroseconds(home);
    digitalWrite(inPosition, LOW);
    red.update(100, 900);
    green.update(100, 900);
    valveTimer = 0;
    return;
  }
  if (digitalRead(posRequest) == HIGH && valve.readMicroseconds() < high - 10) {
    digitalWrite(inPosition, LOW);
    Serial.println("Turning to high");
    valve.writeMicroseconds(high);
    delay(100);
    digitalWrite(inPosition, HIGH);
    red.update(100, 900);
    green.update(0, 1000);
  } 
  if (digitalRead(posRequest) == LOW && valve.readMicroseconds() > low + 10) {
    digitalWrite(inPosition, LOW);
    Serial.println("Turning to low");
    valve.writeMicroseconds(low);
    delay(100);
    digitalWrite(inPosition, HIGH);
    red.update(0, 1000);
    green.update(100, 900);
  }
  valveTimer = 0;
}
