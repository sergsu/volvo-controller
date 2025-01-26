#include <avr/pgmspace.h>

#include "config.h"
#include "config.private.h"
#include "mqtt/at.h"
#include "webasto/control.h"

uint32_t lastReconnectAttempt = 0;

void mqttCallback(String topic, String payload) {
  DEBUGPORT.print("Message arrived [");
  DEBUGPORT.print(topic);
  DEBUGPORT.print("]: ");
  DEBUGPORT.print(payload);
  DEBUGPORT.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicExec) {
    if (payload.startsWith("webasto:")) {
      DEBUGPORT.print("Recognized a Webasto command: ");
      DEBUGPORT.println(payload);
      mqttPublish((char *)topicExecOutput, executeCommand(payload));
    }
  }
}

void mqttSetup() {
  // Set GSM module baud rate
  SerialAT.begin(115200);

  initModem();

  DEBUGPORT.println("Notifying service channel");
  mqttPublish((char *)topicService, "Volvo Controller has started");

  DEBUGPORT.println("Subscribing to MQTT topic");
  // itoa(strlen(topicExec), cstr, 10);
  if (!waitForATResponse("AT+CMQTTSUB=0,21,2,0", ">", 1, 1000)) {
    DEBUGPORT.println(" fail[3]");
    return;
  }
  if (!waitForATResponse(topicExec, "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[4]");
    return;
  }
  DEBUGPORT.println(" success");
}

void mqttLoop() {
  while (SerialAT.available() > 0) {
    char b = SerialAT.read();
    if (b == '\n') {
      continue;
    }
    if (b == '+') {
      String command = SerialAT.readStringUntil('\n');
      if (command.startsWith("CMQTTRXSTART: ")) {
        String serviceInfo = command.substring(14, command.length() - 2);
        int payloadLength =
            serviceInfo.substring(serviceInfo.lastIndexOf(",") + 1).toInt();

        if (!SerialAT.readStringUntil('\n').startsWith("+CMQTTRXTOPIC: 0,")) {
          DEBUGPORT.println("+CMQTTRXTOPIC not found!");
          continue;
        }

        String topic = SerialAT.readStringUntil('\n');
        topic.remove(topic.length() - 1);

        String payload = "";
        do {
          String packetHeader = SerialAT.readStringUntil('\n');
          if (!packetHeader.startsWith("+CMQTTRXPAYLOAD: ")) {
            DEBUGPORT.println("+CMQTTRXPAYLOAD not found!");
            continue;
          }
          int totalPacketLength =
              packetHeader.substring(packetHeader.lastIndexOf(',') + 1).toInt();
          do {
            payload += SerialAT.readStringUntil('\n');
            payload.remove(payload.length() - 1);
          } while (totalPacketLength - payload.length() > 0 &&
                   SerialAT.available() > 0);
        } while (payload.length() < payloadLength);

        if (!SerialAT.readStringUntil('\n').startsWith("+CMQTTRXEND: 0")) {
          DEBUGPORT.println("+CMQTTRXEND not found!");
          continue;
        }

        mqttCallback(topic, payload);
      }
    } else {
      DEBUGPORT.write(b);
    }
  }
  delay(10);
}
