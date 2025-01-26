#include "webasto/utility.h"

enum class State { Unknown, Idle, Burning, LowVoltage };

char *executeCommand(String payload);
State getCurrentState();
State setCurrentState(State _currentState);
State getTargetState();
void setWebastoState(State _targetState);
