params ["_name", "_condition", "_options"];
private _currentOptions = player getVariable ["KH_var_selfInteractionList", []];
_currentOptions pushBack [_name, _condition, _options];
player setVariable ["KH_var_selfInteractionList", _currentOptions];