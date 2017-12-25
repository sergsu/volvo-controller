#include <Arduino.h>

#include "M2tk.h"
#include "m2ghu8g.h"

#include <Time.h>
#include "constants.h"
#include "wbus.h"

wb_sensor_t KeepAlive_wb_sensors;
tmElements_t tm;

uint8_t updateDisplayCounter = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis;

unsigned char heaterMode = WBUS_CMD_OFF;
/* function definitions */
bool heaterOn();
