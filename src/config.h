#ifndef CONFIG_H
#define CONFIG_H

#define DEBUGPORT Serial
#define LED_PIN 13

/**
 * MQTT Configuration
 */
#define SerialAT Serial2
#define TINY_GSM_DEBUG DEBUGPORT

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

static const char apn[] = "YourAPN";
static const char gprsUser[] = "";
static const char gprsPass[] = "";

// MQTT details
static const char* broker = "broker.hivemq.com";

static const char* topicExec = "GsmClientTest/exec";
static const char* topicService = "GsmClientTest/service";
static const char* topicExecOutput = "GsmClientTest/execOutput";

/**
 * Webasto configuration
 */
#define SerialWbus Serial1

static const int LowVoltage = 11.0;
static const int BurnTime = 20;

#endif