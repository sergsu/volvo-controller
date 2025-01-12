#include "mqtt.tinygsm/main.h"
// #include "volvo-p3/main.h"
//#include "webasto/main.h"

void loop() {
  // volvoP3Loop();
#ifdef SerialAT
  mqttLoop();
#endif
  //webastoLoop();
}

void setup() {
  DEBUGPORT.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // volvoP3Setup();
#ifdef SerialAT
  mqttSetup();
#endif
  //webastoSetup();

  digitalWrite(LED_PIN, LOW);
}
