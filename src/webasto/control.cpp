#include "webasto/utility.h"

enum class State { Unknown, Idle, Burning, LowVoltage } currentState;
State targetState = State::Unknown;

void setWebastoState(State _targetState) { targetState = _targetState; }
char *executeCommand(char *payload) {
  if (payload == "webasto:command:start") {
    setWebastoState(State::Burning);
    return "Webasto start has been scheduled";
  } else if (payload == "webasto:command:stop") {
    setWebastoState(State::Idle);
    return "Webasto stop has been scheduled";
  } else if (payload == "webasto:status") {
    float voltage = currentVoltage();

    char buffer[64];
    switch (currentState) {
      case State::Burning:
        sprintf(buffer, "Webasto is running, current voltage: %.2f", voltage);
      case State::Idle:
        sprintf(buffer, "Webasto is idle, current voltage: %.2f", voltage);
      case State::LowVoltage:
        sprintf(buffer,
                "Webasto is disabled due to low voltage, current voltage: %.2f",
                voltage);
      default:
        sprintf(buffer, "Webasto is disabled, current voltage: %.2f", voltage);
    }
    return buffer;
  }
  return "Unknown command";
}