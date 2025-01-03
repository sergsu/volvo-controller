
#include "mqtt/main.h"
#include "webasto/main.h"

void loop() {
  mqttLoop();
  webastoLoop();
}

void setup() {
  mqttSetup();
  webastoSetup();
}
