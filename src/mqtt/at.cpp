#include <Arduino.h>

#include "config.h"
#include "config.private.h"

#define DEBUG_AT_COMMANDS

char cstr[16];
char *strlen_C(char *str) {
  strcpy(cstr, "");
  itoa(strlen(str), cstr, 10);

  return cstr;
}
char *strlen_C_static(char *str) {
  strcpy(cstr, "");
  itoa(strlen_P(str), cstr, 10);

  return cstr;
}

String sendATcommand(const char *toSend, String toWait,
                     unsigned long milliseconds) {
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
  while (millis() - startTime < milliseconds || SerialAT.available()) {
    if (SerialAT.available()) {
      char c = SerialAT.read();
      if (c == 13 || c == 0) {
        continue;
      }
#ifdef DEBUG_AT_COMMANDS
      DEBUGPORT.print(c);
#endif
      result += c;  // append to the result string

      if (toWait.length() > 0 && result.indexOf(toWait) != -1) {
        milliseconds = 0;
      }
    }
  }
#ifdef DEBUG_AT_COMMANDS
  DEBUGPORT.println();  // new line after timeout.
#endif
  return result;
}

String sendATcommandNoWait(const char *toSend, unsigned long milliseconds) {
  return sendATcommand(toSend, String(""), milliseconds);
}

bool waitForATResponse(const char *toSend, String toWait,
                       unsigned long attempts, unsigned long milliseconds) {
  String atResponse;
  int attemptsLeft = attempts;
  do {
    atResponse = sendATcommand(toSend, toWait, milliseconds);
    atResponse.trim();
  } while (atResponse.indexOf(toWait) == -1 && attemptsLeft-- > 0);

  return attemptsLeft > 0 || attempts == 1 && atResponse.indexOf(toWait) != -1;
}

void initModem() {
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
  sendATcommandNoWait("AT+CGDCONT=1,\"IP\",\"\",\"0.0.0.0\",0,0", 200);
  sendATcommandNoWait("AT+CIPMODE=0", 200);
  sendATcommandNoWait("AT+CIPSENDMODE=0", 200);
  sendATcommandNoWait("AT+CIPCCFG=10,0,0,0,1,0,75000", 200);
  sendATcommandNoWait("AT+CIPTIMEOUT=75000,15000,15000", 200);
  sendATcommandNoWait("AT+NETOPEN", 200);

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
  if (!waitForATResponse(strcat("AT+CCERTDOWN=\"cacert.pem\",",
                                strlen_C_static((char *)mqttCert)),
                         ">", 1, 1000)) {
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
  sendATcommandNoWait("AT+CPSI?", 1000);
  if (!waitForATResponse(MQTT_CONNECTION_AT_COMMAND, "OK", 1, 5000)) {
    DEBUGPORT.println(" fail[10]");
    return;
  }
  DEBUGPORT.println(" success");
}

char publishATCommand[256];
bool mqttPublish(char *topic, char *payload) {
  DEBUGPORT.println("Publishing MQTT message");
  sprintf(publishATCommand, "AT+CMQTTTOPIC=0,%d", strlen(topic));
  if (!waitForATResponse(publishATCommand, ">", 1, 1000)) {
    DEBUGPORT.println(" fail[1]");
    return false;
  }
  if (!waitForATResponse(topic, "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[2]");
    return false;
  }
  sprintf(publishATCommand, "AT+CMQTTPAYLOAD=0,%d", strlen(payload));
  if (!waitForATResponse(publishATCommand, ">", 1, 1000)) {
    DEBUGPORT.println(" fail[3]");
    return false;
  }
  if (!waitForATResponse(payload, "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[4]");
    return false;
  }
  if (!waitForATResponse("AT+CMQTTPUB=0,0,60", "OK", 1, 1000)) {
    DEBUGPORT.println(" fail[5]");
    return false;
  }
  DEBUGPORT.println(" success");

  return true;
}
