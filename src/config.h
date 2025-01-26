#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

#define DEBUGPORT Serial
#define LED_PIN 13

/**
 * MQTT Configuration
 */
#define SerialAT Serial1
#define TINY_GSM_DEBUG DEBUGPORT
#define TINY_GSM_USE_GPRS true

/**
 * Webasto configuration
 */
#define SerialWbus Serial2
#define VoltageProbe A0
#define VoltageDividerR1 67.7
#define VoltageDividerR2 19.9

static const int LowVoltage = 11.0;
static const int GeneratorVoltage = 13.5;
static const int BurnTime = 20;

#endif