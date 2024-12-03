params ["_namespace", "_variableName", ["_amount", 1], ["_network", false]];
_namespace setVariable [_variableName, (_namespace getVariable [_variableName, 0]) + _amount, _network];
_namespace getVariable [_variableName, 0];