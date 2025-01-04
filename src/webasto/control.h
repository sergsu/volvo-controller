static enum class State { Unknown, Idle, Burning, LowVoltage } currentState;
static State targetState;

void setWebastoState(State _targetState);
char *executeCommand(char *payload);