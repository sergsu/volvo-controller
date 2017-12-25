
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


void loop() {
  unsigned char incomingByte = 0;
  // send data only when you receive data:
  if (WBUSPORT.available() > 0) {
    // read the incoming byte:
    digitalWrite(49, LOW);
    incomingByte = WBUSPORT.read();
    DEBUGPORT.write(incomingByte);
    digitalWrite(49, HIGH);
  }
}


void setup() {
  //Red LED on
  pinMode(49, OUTPUT);
  digitalWrite(49, HIGH);


  wbus_init();//inits serial 1

  DEBUGPORT.begin(9600);
  //DEBUGPORT.println("Start");
  delay(500);
}
