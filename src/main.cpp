#include "mqtt/main.h"

#include <Sleep_n0m1.h>
#include <avr/power.h>
// #include "volvo-p3/main.h"
#include "Arduino.h"
#include "webasto/main.h"

Sleep sleep;
unsigned long lastActivityTime = millis();
unsigned int activityCooldownTime = 10000U;

void setupLowPower() {
  // A0 is reading voltage
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A6, OUTPUT);
  pinMode(A7, OUTPUT);
  pinMode(A8, OUTPUT);
  pinMode(A9, OUTPUT);
  pinMode(A10, OUTPUT);
  pinMode(A11, OUTPUT);
  // pinMode(A12, OUTPUT);
  // pinMode(A13, OUTPUT);
  // pinMode(A14, OUTPUT);
  // pinMode(A15, OUTPUT);

  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  digitalWrite(A6, LOW);
  digitalWrite(A7, LOW);
  digitalWrite(A8, LOW);
  digitalWrite(A9, LOW);
  digitalWrite(A10, LOW);
  digitalWrite(A11, LOW);
  // digitalWrite(A12, LOW);
  // digitalWrite(A13, LOW);
  // digitalWrite(A14, LOW);
  // digitalWrite(A15, LOW);

  for (int i = 2; i <= 53; i++) {
    if (i == 16 || i == 17 || i == 18 || i == 19) {
      continue;  // Serial ports actually used
    }
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // power_usart3_disable();
  // power_timer1_disable();
  // power_timer2_disable();
  // power_timer3_disable();
  // power_timer4_disable();
  // power_timer5_disable();
  // power_twi_disable();
}

void loop() {
  // DEBUGPORT.print(".");
  // digitalWrite(LED_PIN, HIGH);

  // if (SerialAT.available() || SerialWbus.available()) {
  //   lastActivityTime = millis();
  // }

  // volvoP3Loop();
  // mqttLoop();
  webastoLoop();

  // digitalWrite(LED_PIN, LOW);
  // sleep.idleMode();
  // sleep.sleepDelay(isActivityCooldownPeriod() ? 10000 : 500);
  delay(20);
}

bool isActivityCooldownPeriod() {
  return millis() > lastActivityTime + activityCooldownTime;
}

void setup() {
  DEBUGPORT.begin(115200);

  // setupLowPower();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // volvoP3Setup();
  // mqttSetup();
  webastoSetup();

  digitalWrite(LED_PIN, LOW);
}
