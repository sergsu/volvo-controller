#define TINY_GSM_MODEM_A7672X

#include <PubSubClient.h>
#include <TinyGsmClient.h>

#include "../config.h"

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char *topic, byte *payload, unsigned int len) {
  DEBUGPORT.print("Message arrived [");
  DEBUGPORT.print(topic);
  DEBUGPORT.print("]: ");
  DEBUGPORT.write(payload, len);
  DEBUGPORT.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}

boolean mqttConnect() {
  DEBUGPORT.print("Connecting to ");
  DEBUGPORT.print(broker);

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  // boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

  if (status == false) {
    DEBUGPORT.println(" fail");
    return false;
  }
  DEBUGPORT.println(" success");
  mqtt.publish(topicInit, "GsmClientTest started");
  mqtt.subscribe(topicLed);
  return mqtt.connected();
}

void mqttSetup() {
  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  DEBUGPORT.println("Wait...");

  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  // SerialAT.begin(9600);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DEBUGPORT.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  DEBUGPORT.print("Modem Info: ");
  DEBUGPORT.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }
#endif

#if TINY_GSM_USE_WIFI
  // Wifi connection parameters must be set before waiting for the network
  DEBUGPORT.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    DEBUGPORT.println(" fail");
    delay(10000);
    return;
  }
  DEBUGPORT.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  DEBUGPORT.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    DEBUGPORT.println(" fail");
    return;
  }
  DEBUGPORT.println(" success");

  if (modem.isNetworkConnected()) {
    DEBUGPORT.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  DEBUGPORT.print(F("Connecting to "));
  DEBUGPORT.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    DEBUGPORT.println(" fail");
    return;
  }
  DEBUGPORT.println(" success");

  if (modem.isGprsConnected()) {
    DEBUGPORT.println("GPRS connected");
  }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void mqttLoop() {
  // Make sure we're still registered on the network
  if (!modem.isNetworkConnected()) {
    DEBUGPORT.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true)) {
      DEBUGPORT.println(" fail");
      return;
    }
    if (modem.isNetworkConnected()) {
      DEBUGPORT.println("Network re-connected");
    }

#if TINY_GSM_USE_GPRS
    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
      DEBUGPORT.println("GPRS disconnected!");
      DEBUGPORT.print(F("Connecting to "));
      DEBUGPORT.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        DEBUGPORT.println(" fail");
        return;
      }
      if (modem.isGprsConnected()) {
        DEBUGPORT.println("GPRS reconnected");
      }
    }
#endif
  }

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

  mqtt.loop();
}