
#include "mqtt/main.h"

#include "webasto/main.h"

void loop() {
  mqttLoop();
  webastoLoop();
}

void setup() {
  DEBUGPORT.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  mqttSetup();
  webastoSetup();

  digitalWrite(LED_PIN, LOW);
}
