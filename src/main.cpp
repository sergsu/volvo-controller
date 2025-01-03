
#include "mqtt/main.h"
#include "volvo-p3/main.h"
#include "webasto/main.h"

void loop() {
  volvoP3Loop();
  mqttLoop();
  webastoLoop();
}

void setup() {
  DEBUGPORT.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  volvoP3Setup();
  mqttSetup();
  webastoSetup();

  digitalWrite(LED_PIN, LOW);
}
