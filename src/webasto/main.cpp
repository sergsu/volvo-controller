
/*
  WebastoHeaterWBusArduinoInterface

   Copyright 2015 Stuart Pittaway

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Original idea and work by Manuel Jander  mjander@users.sourceforge.net
   https://sourceforge.net/projects/libwbus/
*/

// __TIME__ __DATE__

#include "webasto/main.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "config.h"
#include "config.private.h"
#include "mqtt/at.h"
#include "webasto/control.h"
#include "webasto/utility.h"

WbusInterface* wbusHeater;
uint8_t cmd = 0;
uint8_t data[10];
int dlen = 0;
int skip = 0;
uint8_t addr = 0;
int err = 0;
int normalVoltageDesiredCycles = 0;
unsigned long startTime = 0;
unsigned long lastVoltageCheck = 0;
unsigned long voltageCheckInterval = 1000;
unsigned long lastKeepAlive = 0;
unsigned long keepAliveInterval = 2000;

bool isVoltageNormal() {
  float voltage = currentVoltage();
  DEBUGPORT.print("check Voltage: ");
  DEBUGPORT.println(voltage);
  if (voltage < LowVoltage) {
    return false;
  }
  return true;
}

bool isCarRunning() {
  float voltage = currentVoltage();
  DEBUGPORT.print("check Voltage: ");
  DEBUGPORT.println(voltage);
  if (voltage > GeneratorVoltage) {
    return false;
  }
  return true;
}

bool shutdownHeater() {
  startTime = 0;
  DEBUGPORT.println("off");
  if (!wbusHeater->turnOff()) {
    mqttPublish((char*)topicService, "Webasto has shut down.");
    return true;
  }
  mqttPublish((char*)topicService, "Webasto did not shutdown!");
  return false;
}

bool startHeater() {
  if (wbusHeater->turnOn(WBUS_CMD_ON_PH, BurnTime)) {
    DEBUGPORT.println("startHeater(): rcv err heater");
    mqttPublish((char*)topicService, "Webasto did not start!");
    return false;
  }

  mqttPublish((char*)topicService, "Webasto has started.");
  startTime = millis();

  return true;
}

bool keepAlive() {
  if (lastKeepAlive + keepAliveInterval > millis()) {
    return true;
  }

  if (wbusHeater->check(WBUS_CMD_ON_PH)) {
    DEBUGPORT.println("keepAlive(): rcv err heater");
    return false;
  }
  lastKeepAlive = millis();

  return true;
}

bool checkBurnTime() {
  unsigned long burnTime = BurnTime * 60ul * 1000ul;  // mins to millis
  if (millis() - startTime > burnTime) {
    DEBUGPORT.println("BurnTime over");
    return false;
  }
  DEBUGPORT.print((burnTime - (millis() - startTime)) / 1000l);
  DEBUGPORT.println(" RestZeit");
  return true;
}

void webastoLoop() {
  // unsigned char incomingByte = 0;
  // if (SerialWbus.available() > 0) {
  //   // read the incoming byte:
  //   incomingByte = SerialWbus.read();
  //   DEBUGPORT.write(incomingByte);
  // }

  switch (getCurrentState()) {
    case State::Idle:
      if (getTargetState() == State::Burning && isVoltageNormal()) {
        digitalWrite(LED_PIN, HIGH);
        if (startHeater()) {
          setCurrentState(State::Burning);
        }
        digitalWrite(LED_PIN, LOW);
      }
      break;
    case State::Burning:
      if (!isVoltageNormal()) {
        if (shutdownHeater()) {
          setCurrentState(State::LowVoltage);
        }
      } else if (!checkBurnTime()) {
        if (shutdownHeater()) {
          setCurrentState(State::Idle);
        }
      } else if (isCarRunning()) {
        if (shutdownHeater()) {
          setCurrentState(State::Idle);
        }
      } else if (getTargetState() == State::Idle) {
        if (shutdownHeater()) {
          setCurrentState(State::Idle);
        }
      } else {
        keepAlive();
      }
      break;
    case State::LowVoltage:
      if (millis() > lastVoltageCheck + voltageCheckInterval) {
        if (isVoltageNormal()) {
          if (normalVoltageDesiredCycles > 0) {
            --normalVoltageDesiredCycles;
          } else {
            setCurrentState(State::Idle);
            normalVoltageDesiredCycles = 5;
          }
          lastVoltageCheck = millis();
        } else {
          normalVoltageDesiredCycles = 5;
        }
      }
      break;
    case State::Unknown:
      setCurrentState(State::Idle);
      break;
    default:
      DEBUGPORT.println("Undefined State!");
  }
}

void webastoSetup() {
  // Red LED on
  setCurrentState(State::Unknown);
  wbusHeater = new WbusInterface(SerialWbus);

  setCurrentState(State::Idle);

  char buf[32];
  sprintf(buf, "Current voltage: %.1f V", currentVoltage());
  mqttPublish((char*)topicService, buf);
}
