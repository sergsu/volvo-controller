#include <avr/pgmspace.h>

#include "config.h"
#include "config.private.h"
// #include "webasto/control.h"

#define DEBUG_AT_COMMANDS

uint32_t lastReconnectAttempt = 0;
char cstr[16];

void mqttCallback(char *topic, byte *payload, unsigned int len) {
  DEBUGPORT.print("Message arrived [");
  DEBUGPORT.print(topic);
  DEBUGPORT.print("][");
  DEBUGPORT.print(len);
  DEBUGPORT.print("]: ");
  DEBUGPORT.print((String)(char *)payload);
  DEBUGPORT.println();

  // Only proceed if incoming message's topic matches
  // if (String(topic) == topicExec) {
  //   if (((String)(char *)payload).startsWith("w:")) {
  //     DEBUGPORT.print("Recognized a Webasto command");
  //     // mqtt.publish(topicExecOutput, executeCommand((char *)payload));
  //   }
  // }
}

String sendATcommand(const char *toSend, unsigned long milliseconds) {
  String result;
#ifdef DEBUG_AT_COMMANDS
  DEBUGPORT.print("Sending: ");
  DEBUGPORT.println(toSend);
#endif
  SerialAT.println(toSend);
  unsigned long startTime = millis();
#ifdef DEBUG_AT_COMMANDS
  DEBUGPORT.print("Received: ");
#endif
  while (millis() - startTime < milliseconds) {
    if (SerialAT.available()) {
      char c = SerialAT.read();
      if (c == 13 || c == 0) {
        continue;
      }
#ifdef DEBUG_AT_COMMANDS
      DEBUGPORT.print(c);
#endif
      result += c;  // append to the result string
    }
  }
#ifdef DEBUG_AT_COMMANDS
  DEBUGPORT.println();  // new line after timeout.
#endif
  return result;
}

bool waitForATResponse(const char *toSend, String toWait,
                       unsigned long attempts, unsigned long milliseconds) {
  String atResponse;
  int attemptsLeft = attempts;
  do {
    atResponse = sendATcommand(toSend, milliseconds);
    atResponse.trim();
  } while (atResponse.indexOf(toWait) == -1 && attemptsLeft-- > 0);

  return attemptsLeft > 0 || attempts == 1 && atResponse.indexOf(toWait) != -1;
}

void mqttSetup() {
  // Set GSM module baud rate
  SerialAT.begin(115200);

  DEBUGPORT.println("Initializing modem...");
  if (!waitForATResponse("AT+CRESET", "OK", 1, 9000)) {
    DEBUGPORT.println("Reset failed");
    return;
  }
  if (!waitForATResponse("ATE0", "OK", 100, 2000)) {
    DEBUGPORT.println("Echo mode setting failed");
    return;
  }

  DEBUGPORT.println("Waiting for network...");
  if (!waitForATResponse("AT+CGREG?", "+CGREG: 0,1", 100, 250)) {
    DEBUGPORT.println(" fail");
    return;
  }
  DEBUGPORT.println(" success");

  DEBUGPORT.println("Connecting to internet...");
  sendATcommand("AT+CGDCONT=1,\"IP\",\"\",\"0.0.0.0\",0,0", 200);
  sendATcommand("AT+CIPMODE=0", 200);
  sendATcommand("AT+CIPSENDMODE=0", 200);
  sendATcommand("AT+CIPCCFG=10,0,0,0,1,0,75000", 200);
  sendATcommand("AT+CIPTIMEOUT=75000,15000,15000", 200);
  sendATcommand("AT+NETOPEN", 200);

  if (!waitForATResponse("AT+NETOPEN?", "+NETOPEN: 1", 150, 500)) {
    DEBUGPORT.println(" fail[1]");
    return;
  }
  if (!waitForATResponse("AT+CGACT?", "+CGACT: 1,1", 150, 500)) {
    DEBUGPORT.println(" fail[2]");
    return;
  }
  DEBUGPORT.println(" success");

  DEBUGPORT.println("Configuring SSL");
  itoa(strlen_P(mqttCert), cstr, 10);
  if (!waitForATResponse(strcat("AT+CCERTDOWN=\"cacert.pem\",", cstr), ">", 1,
                         1000)) {
    DEBUGPORT.println(" fail[1]");
    return;
  }

  char certChar;
  for (uint16_t k = 0; k < strlen_P(mqttCert); k++) {
    certChar = pgm_read_byte_near(mqttCert + k);
    SerialAT.print(certChar);
  }
  SerialAT.println();

  DEBUGPORT.println(" success");

  DEBUGPORT.println("Connecting to MQTT broker");
  if (!waitForATResponse("AT+CSSLCFG=\"sslversion\",0,4", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[1]");
    return;
  }
  if (!waitForATResponse("AT+CSSLCFG=\"authmode\",0,1", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[2]");
    return;
  }
  if (!waitForATResponse("AT+CSSLCFG=\"cacert\",0,\"cacert.pem\"", "OK", 1,
                         1000)) {
    DEBUGPORT.println(" fail[3]");
    return;
  }
  if (!waitForATResponse("AT+CSSLCFG=\"enableSNI\",0,1", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[4]");
    return;
  }

  if (!waitForATResponse("AT+CMQTTSTART", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[7]");
    return;
  }
  if (!waitForATResponse("AT+CMQTTACCQ=0,\"volvo-controller\",1", "OK", 1,
                         1000)) {
    DEBUGPORT.println(" fail[8]");
    return;
  }
  if (!waitForATResponse("AT+CMQTTSSLCFG=0,0", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[9]");
    return;
  }
  sendATcommand("AT+CPSI?", 1000);
  if (!waitForATResponse(MQTT_CONNECTION_AT_COMMAND, "OK", 1, 5000)) {
    DEBUGPORT.println(" fail[10]");
    return;
  }
  DEBUGPORT.println(" success");

  DEBUGPORT.println("Notifying service channel");
  // itoa(strlen(topicService), cstr, 10);
  if (!waitForATResponse("AT+CMQTTTOPIC=0,24", ">", 1, 1000)) {
    DEBUGPORT.println(" fail[1]");
    return;
  }
  if (!waitForATResponse(topicService, "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[2]");
    return;
  }
  if (!waitForATResponse("AT+CMQTTPAYLOAD=0,9", ">", 1, 1000)) {
    DEBUGPORT.println(" fail[3]");
    return;
  }
  if (!waitForATResponse("connected", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[4]");
    return;
  }
  if (!waitForATResponse("AT+CMQTTPUB=0,0,60", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[5]");
    return;
  }
  DEBUGPORT.println(" success");

  DEBUGPORT.println("Subscribing to MQTT topic");
  // itoa(strlen(topicExec), cstr, 10);
  if (!waitForATResponse("AT+CMQTTSUB=0,21,0", ">", 1, 1000)) {
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
  // Make sure we're still registered on the network
  //   if (!modem.isNetworkConnected()) {
  //     DEBUGPORT.println("Network disconnected");
  //     if (!modem.waitForNetwork(1000L, true)) {
  //       DEBUGPORT.println(" fail");
  //       return;
  //     }
  //     if (modem.isNetworkConnected()) {
  //       DEBUGPORT.println("Network re-connected");
  //     }

  // #if TINY_GSM_USE_GPRS
  //     // and make sure GPRS/EPS is still connected
  //     if (!modem.isGprsConnected()) {
  //       DEBUGPORT.println("GPRS disconnected!");
  //       DEBUGPORT.print(F("Connecting to "));
  //       DEBUGPORT.print(apn);
  //       if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
  //         DEBUGPORT.println(" fail");
  //         return;
  //       }
  //       if (modem.isGprsConnected()) {
  //         DEBUGPORT.println("GPRS reconnected");
  //       }
  //     }
  // #endif
  //   }

  //   if (!mqtt.connected()) {
  //     // Reconnect every 10 seconds
  //     uint32_t t = millis();
  //     if (t - lastReconnectAttempt > 10000L) {
  //       DEBUGPORT.println("=== MQTT NOT CONNECTED ===");
  //       lastReconnectAttempt = t;
  //       if (mqttConnect()) {
  //         lastReconnectAttempt = 0;
  //       } else {
  //         return;
  //       }
  //     }
  //   }

  //   mqtt.loop();

  DEBUGPORT.print('.');
  while (SerialAT.available() > 0) {
    DEBUGPORT.write(SerialAT.read());
  }
  delay(1000);
}
