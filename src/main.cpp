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
const int lowus=1205; //0 degrees 1205 us
const int highus=1795; //179 degrees 1795 us
const int low=0; //0 degrees 1205 us
const int home=90; //0 degrees 1205 us
const int high=179; //0 degrees 1205 us

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
  pinMode(posRequest, INPUT);
  pinMode(inPosition, OUTPUT);
  valve.attach(1, lowus, highus);
  valve.write(home);
  red.update(100, 900);
  green.update(100, 900);
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
    valve.write(home);
    digitalWrite(inPosition, LOW);
    red.update(100, 900);
    green.update(100, 900);
    valveTimer = 0;
    return;
  }
  if (digitalRead(posRequest) == HIGH && valve.read() < high - 10) {
    digitalWrite(inPosition, LOW);
    Serial.println("Turning to high");
    valve.write(high);
    delay(100);
    digitalWrite(inPosition, HIGH);
    red.update(100, 900);
    green.update(0, 1000);
  } 
  if (digitalRead(posRequest) == LOW && valve.read() > low + 10) {
    digitalWrite(inPosition, LOW);
    Serial.println("Turning to low");
    valve.write(low);
    delay(100);
    digitalWrite(inPosition, HIGH);
    red.update(0, 1000);
    green.update(100, 900);
  }
  valveTimer = 0;
}
