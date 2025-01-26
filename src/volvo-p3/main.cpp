/**
 * Car control module
 *
 * Ideas of features:
 * - Use the remote control keyfob to close the automatic tailgate
 * - Force switch to Video2 when reverse gear is de-selected until speed reaches
 * 10 km/h (forward facing CAM)
 * - Force switch to Video2 when front parking sensor becomes active
 * - Telemetry data sent every minute
 *
 * @link https://github.com/sparkfun/SparkFun_CAN-Bus_Arduino_Library
 * @link https://www.swedesolutions.com/cfe-capabilities/
 * @link https://d5t5.com/article/vdash-vdd-dongle
 */

#include <Canbus.h>
#include <SPI.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>

#include "config.h"

void sendMessage() {
  tCAN message;

  message.id = 0x631;  // formatted in HEX
  message.header.rtr = 0;
  message.header.length = 8;  // formatted in DEC
  message.data[0] = 0x40;
  message.data[1] = 0x05;
  message.data[2] = 0x30;
  message.data[3] = 0xFF;  // formatted in HEX
  message.data[4] = 0x00;
  message.data[5] = 0x40;
  message.data[6] = 0x00;
  message.data[7] = 0x00;

  mcp2515_bit_modify(CANCTRL, (1 << REQOP2) | (1 << REQOP1) | (1 << REQOP0), 0);
  mcp2515_send_message(&message);
}

void volvoP3Setup() {
  DEBUGPORT.println("CAN Read - Testing receival of CAN Bus message");
  delay(1000);

  if (Canbus.init(CANSPEED_500))  // Initialise MCP2515 CAN controller at the
                                  // specified speed
    DEBUGPORT.println("CAN Init ok");
  else
    DEBUGPORT.println("Can't init CAN");
}

void volvoP3Loop() {
  tCAN message;
  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {
      // if(message.id == 0x620 and message.data[2] == 0xFF)
      // uncomment when you want to filter
      //{

      DEBUGPORT.print("ID: ");
      DEBUGPORT.print(message.id, HEX);
      DEBUGPORT.print(", ");
      DEBUGPORT.print("Data: ");
      DEBUGPORT.print(message.header.length, DEC);
      for (int i = 0; i < message.header.length; i++) {
        DEBUGPORT.print(message.data[i], HEX);
        DEBUGPORT.print(" ");
      }
      DEBUGPORT.println("");
      //}
    }
  }
  sendMessage();
}
