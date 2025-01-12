#include <HardwareSerial.h>

#include "config.h"

void mqttSetup();
void mqttLoop();

String sendATcommand(const char *toSend, unsigned long milliseconds);
