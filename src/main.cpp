
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

WbusInterface* wbusRadio;
WbusInterface* wbusHeater;
uint8_t cmd = 0;
uint8_t in = 0;
int dlen = 0;
int skip = 0;
uint8_t addr = 0;
int err = 0;

void loop() {
  digitalWrite(49, LOW);
  cmd = 0; in = 0; dlen = 0; skip = 0; addr = 0x24; //radio button sends to heater
  err = wbusRadio->listen( &addr, &cmd, &in, &dlen);
  if(err){
    DEBUGPORT.print(".");
    return;
  }

  DEBUGPORT.print("radio said: ");
  DEBUGPORT.print(cmd, HEX);
  DEBUGPORT.println(in, HEX);

  if(cmd == WBUS_CMD_ON){
    cmd = WBUS_CMD_ON_PH;
    DEBUGPORT.println("heater cmd exchanged");
    //in = 0x16; //limit to 20 min
  }
  digitalWrite(49, HIGH);
  err = wbusHeater->io( &cmd, &in, nullptr, 0, &in, &dlen, skip);
  if(err){
    DEBUGPORT.println("rcv err heater");
    return;
  }
  DEBUGPORT.print("heater answered: ");
  DEBUGPORT.println(in, HEX);

  if( (cmd & 0x7F) == WBUS_CMD_ON_PH ){
    cmd = (cmd & 0x80) | WBUS_CMD_ON;
    DEBUGPORT.println("heater cmd answer exchanged");
  }
  wbusRadio->send( addr, cmd, &in, dlen, nullptr, 0);
  if(err){
    DEBUGPORT.println("answer to radio err");
    return;
  }
}

void setup() {
  //Red LED on
  DEBUGPORT.begin(9600);
  wbusRadio = new WbusInterface(Serial1);
  wbusHeater = new WbusInterface(Serial2);
  pinMode(49, OUTPUT);
  digitalWrite(49, HIGH);
}
