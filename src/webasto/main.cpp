
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

bool waitForOnSignal() {
  // cmd = 0; dlen = 0; addr = 0x24; //radio button sends to heater
  // startTime = 0;
  // int err = wbusRadio->listen( &addr, &cmd, data, &dlen);
  // if(err){
  //   DEBUGPORT.print(".");
  //   return false;
  // }

  // if(cmd == WBUS_CMD_ON || cmd == WBUS_CMD_ON_PH ||cmd == WBUS_CMD_ON_SH){
  //   wbusRadio->send( 0x42, cmd, data, dlen, nullptr, 0);
  //   return true;
  // }
  return false;
}

bool waitForOffSignal() {
  // cmd = 0; dlen = 0; addr = 0x24; //radio button sends to heater
  // int err = wbusRadio->listen( &addr, &cmd, data, &dlen);
  // if(err){
  //   DEBUGPORT.print(":");
  //   return false;
  // }

  // if(cmd == WBUS_CMD_OFF){
  //   startTime = 0;
  //   wbusRadio->send( 0x42, cmd, data, dlen, nullptr, 0);
  //   return true;
  // }
  return false;
}

bool isVoltageNormal() {
  float voltage = currentVoltage();
  DEBUGPORT.print(voltage);
  DEBUGPORT.println("check Voltage");
  if (voltage < LowVoltage) {
    return false;
  }
  return true;
}

bool shutdownHeater() {
  startTime = 0;
  DEBUGPORT.println("off");
  if (!wbusHeater->turnOff()) {
    return true;
  }
  return false;
}

bool startHeater() {
  if (wbusHeater->turnOn(WBUS_CMD_ON_PH, BurnTime)) {
    DEBUGPORT.println("startHeater(): rcv err heater");
    return false;
  }

  startTime = millis();
  return true;
}

bool keepAlive() {
  if (wbusHeater->check(WBUS_CMD_ON_PH)) {
    DEBUGPORT.println("keepAlive(): rcv err heater");
    return false;
  }
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
#ifdef SerialWbus
  unsigned char incomingByte = 0;
  if (SerialWbus.available() > 0) {
    // read the incoming byte:
    digitalWrite(49, LOW);
    incomingByte = SerialWbus.read();
    DEBUGPORT.write(incomingByte);
  }
#endif

  switch (currentState) {
    case State::Idle:
      if (targetState == State::Burning && isVoltageNormal()) {
        digitalWrite(LED_PIN, HIGH);
        if (startHeater()) {
          currentState = State::Burning;
        }
        digitalWrite(LED_PIN, LOW);
      }
      break;
    case State::Burning:
      if (!isVoltageNormal()) {
        if (shutdownHeater()) {
          currentState = State::LowVoltage;
        }
      } else if (!checkBurnTime()) {
        if (shutdownHeater()) {
          currentState = State::Idle;
        }
      } else if (targetState == State::Idle) {
        if (shutdownHeater()) {
          currentState = State::Idle;
        }
      } else {
        keepAlive();
      }
      break;
    case State::LowVoltage:
      if (millis() >
          lastVoltageCheck + 1000u) {  // only check once a second at most
        if (isVoltageNormal()) {
          if (normalVoltageDesiredCycles > 0) {
            --normalVoltageDesiredCycles;
          } else {
            currentState = State::Idle;
            normalVoltageDesiredCycles = 5;
          }
          lastVoltageCheck = millis();
        } else {
          normalVoltageDesiredCycles = 5;
        }
      }
      break;
    case State::Unknown:
    default:
      DEBUGPORT.println("Undefined State!");
  }
}

void webastoSetup() {
  // Red LED on
  currentState = State::Unknown;
#ifdef SerialWbus
  wbusHeater = new WbusInterface(SerialWbus);
#endif

  currentState = State::Idle;
}
