#define TINY_GSM_MODEM_A7672X

#include <PubSubClient.h>
#include <TinyGsmClient.h>

#include "config.h"
#include "config.private.h"
#include "webasto/control.h"

TinyGsm modem(SerialAT);
TinyGsmClientSecure client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char *topic, byte *payload, unsigned int len) {
  DEBUGPORT.print("Message arrived [");
  DEBUGPORT.print(topic);
  DEBUGPORT.print("]: ");
  DEBUGPORT.write(payload, len);
  DEBUGPORT.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicExec) {
    if (((String)(char *)payload).startsWith("webasto:")) {
      DEBUGPORT.println("Recognized a Webasto command");
      DEBUGPORT.println(strlen((char *)payload));
      DEBUGPORT.println(executeCommand((char *)payload));
      mqtt.publish(topicExecOutput, executeCommand((char *)payload), 64);
    }
  }
}

boolean mqttConnect() {
  DEBUGPORT.print("Connecting to ");
  DEBUGPORT.print(mqttBroker);

  boolean status =
      mqtt.connect(mqttAuthUser, mqttAuthUser, mqttAuthPassword);

  if (status == false) {
    DEBUGPORT.println(" fail");
    return false;
  }
  DEBUGPORT.println(" success");
  mqtt.publish(topicService, "Volvo Controller started. ");
  DEBUGPORT.println(executeCommand("webasto:status"));
  mqtt.publish(topicService, executeCommand("webasto:status"), 64);
  mqtt.subscribe(topicExec);
  return mqtt.connected();
}

void mqttSetup() {
  DEBUGPORT.println("Wait...");

  // Set GSM module baud rate
  SerialAT.begin(115200);

  // Restart takes quite some time
  DEBUGPORT.println("Initializing modem...");
  modem.restart();

  DEBUGPORT.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    DEBUGPORT.println(" fail");
    return;
  }
  DEBUGPORT.println(" success");

  modem.addCertificate("cacert.pem", mqttCert, strlen_P(mqttCert));
  modem.setCertificate("cacert.pem");

  if (modem.isNetworkConnected()) {
    DEBUGPORT.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  DEBUGPORT.print(F("Connecting to GPRS"));;
  if (!modem.gprsConnect("", "", "")) {
    DEBUGPORT.println(" fail");
    return;
  }
  DEBUGPORT.println(" success");
#endif

  // MQTT Broker setup
  mqtt.setServer(mqttBroker, mqttPort);
  mqtt.setCallback(mqttCallback);
}

void mqttLoop() {
  // Make sure we're still registered on the network
//   if (!modem.isNetworkConnected()) {
//     DEBUGPORT.println("Network disconnected");
//     if (!modem.waitForNetwork(180000L, true)) {
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
//       DEBUGPORT.print(F("Connecting to GPRS"));
//       if (!modem.gprsConnect("", "", "")) {
//         DEBUGPORT.println(" fail");
//         return;
//       }
//     }
// #endif
//   }

  if (!mqtt.connected()) {
    DEBUGPORT.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    return;
  }

  if (!mqtt.loop()) {
    DEBUGPORT.println("MQTT loop failed");
  }
}