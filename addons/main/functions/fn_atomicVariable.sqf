params ["_namespace", "_variableName", ["_variableValue", false], ["_network", false]];
private _atomicName = format ["KH_var_atomic_%1", _variableName];
private _atomicIncrement = (_namespace getVariable [_atomicName, 0]) + 1;
private _atomicVariable = format ["%1%2", _variableName, _atomicIncrement];
_namespace setVariable [_atomicName, _atomicIncrement, _network];
_namespace setVariable [_atomicVariable, _variableValue, _network];
_atomicVariable;