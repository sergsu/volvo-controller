enum class State { Unknown, Idle, Burning, LowVoltage } currentState;
State targetState = State::Unknown;

void setWebastoState(State _targetState) { targetState = _targetState; }