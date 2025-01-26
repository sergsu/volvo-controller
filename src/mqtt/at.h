#include <Arduino.h>

String sendATcommandNoWait(const char *toSend, unsigned long milliseconds);
String sendATcommand(const char *toSend, String toWait,
                     unsigned long milliseconds);
bool waitForATResponse(const char *toSend, String toWait,
                       unsigned long attempts, unsigned long milliseconds);
void initModem();
bool mqttPublish(char *topic, char *payload);