#include "config.h"
#include "webasto/utility.h"

enum class State { Unknown, Idle, Burning, LowVoltage };

State currentState = State::Idle;
State targetState = State::Idle;

State getTargetState() { return targetState; }
void setWebastoState(State _state) { targetState = _state; }

void setCurrentState(State _state) { currentState = _state; }
State getCurrentState() { return currentState; }

char buffer[32];
char* executeCommand(String payload) {
  if (payload == "webasto:command:start") {
    setWebastoState(State::Burning);
    return "Webasto start has been scheduled";
  } else if (payload == "webasto:command:stop") {
    setWebastoState(State::Idle);
    return "Webasto stop has been scheduled";
  } else if (payload == "webasto:status") {
    float voltage = currentVoltage();

    switch (currentState) {
      case State::Burning:
        sprintf(buffer, "Webasto is running, current voltage: %.1f V", voltage);
      case State::Idle:
        sprintf(buffer, "Webasto is idle, current voltage: %.1f V", voltage);
      case State::LowVoltage:
        sprintf(
            buffer,
            "Webasto is disabled due to low voltage, current voltage: %.1f V",
            voltage);
      default:
        sprintf(buffer, "Webasto is disabled, current voltage: %.1f V",
                voltage);
    }
    return buffer;
  }
  return "Unknown command";
}