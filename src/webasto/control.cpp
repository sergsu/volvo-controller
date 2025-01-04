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
    switch (currentState) {
      case State::Burning:
        return "Webasto is running";
      case State::Idle:
        return "Webasto is idle";
      case State::LowVoltage:
        return "Webasto is disabled due to low voltage";
      default:
        return "Webasto is disabled";
    }
  }
  return "Unknown command";
}