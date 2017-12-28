
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


#include <Arduino.h>
#include <EEPROM.h>
#include "utility.h"

#include "main.h"

const int LowVoltage = 11.0;

WbusInterface* wbusRadio;
WbusInterface* wbusHeater;
BurnTimePresetInput* presetInput;
uint8_t cmd = 0;
uint8_t data[10];
int dlen = 0;
int skip = 0;
uint8_t addr = 0;
int err = 0;
int voltageCheckCnt = 0;
unsigned long startTime = 0;

enum class State{
  Unknown,
  Idle,
  Burning,
  LowVoltage
} currentState;

bool waitForOnSignal(){
  cmd = 0; dlen = 0; addr = 0x24; //radio button sends to heater
  startTime = 0;
  int err = wbusRadio->listen( &addr, &cmd, data, &dlen);
  if(err){
    DEBUGPORT.print(".");
    return false;
  }

  if(cmd == WBUS_CMD_ON || cmd == WBUS_CMD_ON_PH ||cmd == WBUS_CMD_ON_SH){
    wbusRadio->send( 0x42, cmd, data, dlen, nullptr, 0);
    return true;
  }
  return false;
}

bool waitForOffSignal(){
  cmd = 0; dlen = 0; addr = 0x24; //radio button sends to heater
  int err = wbusRadio->listen( &addr, &cmd, data, &dlen);
  if(err){
    DEBUGPORT.print(":");
    return false;
  }

  if(cmd == WBUS_CMD_OFF){
    startTime = 0;
    wbusRadio->send( 0x42, cmd, data, dlen, nullptr, 0);
    return true;
  }
  return false;
}

bool checkVoltage(){
  constexpr float voltageDivider = (17.8+66.0)/17.8;//voltage divisor resistors
  constexpr float scale = 12.2/12.7;//correction
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (voltageDivider*scale*5.0 / 1023.0);
  DEBUGPORT.print(voltage);
  DEBUGPORT.println("check Voltage");
  if(voltage<LowVoltage){
    return false;
  }
  return true;
}

bool shutdownHeater(){
  startTime = 0;
  DEBUGPORT.println("off");
  if(!wbusHeater->turnOff()){
    return true;
  }
  return false;
}

bool startHeater(){
  if(wbusHeater->turnOn(WBUS_CMD_ON_PH, 20)){
    DEBUGPORT.println("startHeater(): rcv err heater");
    return false;
  }

  startTime = millis();
  return true;
}

bool keepAlive(){
  if(wbusHeater->check(WBUS_CMD_ON_PH)){
    DEBUGPORT.println("keepAlive(): rcv err heater");
    return false;
  }
  return true;
}

bool checkBurnTime(){
  unsigned long burnTime = presetInput->getBurnTime() * 60ul * 1000ul;//mins to millis
  if(millis() - startTime > burnTime){
    DEBUGPORT.println("BurnTime over");
    return false;
  }
  DEBUGPORT.print((burnTime - (millis()-startTime)) / 1000l);
  DEBUGPORT.println(" RestZeit");
  return true;
}

void loop() {
  switch(currentState){
    case State::Idle:
      if(waitForOnSignal() && checkVoltage()){
        digitalWrite(53, HIGH);
        if(startHeater()){
          currentState = State::Burning;
        }
      }
      digitalWrite(53, LOW);
    break;
    case State::Burning:
      if(!checkVoltage()){
        if(shutdownHeater()){
          currentState = State::LowVoltage;
        }
      }else if(!checkBurnTime()){
        if(shutdownHeater()){
          currentState = State::Idle;
        }
      }else if(waitForOffSignal()){
        if(shutdownHeater()){
          currentState = State::Idle;
        }
      }else{
        keepAlive();
      }
    break;
    case State::LowVoltage:
      if(checkVoltage()){
        if(voltageCheckCnt>0){
          --voltageCheckCnt;
          delay(600);//todo: set to 60000
        }else{
          currentState = State::Idle;
          voltageCheckCnt = 5;
        }
      }else{
        voltageCheckCnt = 5;//voltage must be 5 min ok
        delay(600);//todo: set to 60000
      }
    break;
    case State::Unknown:
    default:
      DEBUGPORT.println("Undefined State!");
  }
}

void setup() {
  //Red LED on
  currentState = State::Unknown;
  DEBUGPORT.begin(9600);
  wbusRadio = new WbusInterface(Serial1);
  wbusHeater = new WbusInterface(Serial2);
  presetInput = new BurnTimePresetInput();
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);

  currentState = State::Idle;
}
