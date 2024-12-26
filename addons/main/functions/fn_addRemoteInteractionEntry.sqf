params ["_entity", "_name", "_condition", "_options"];
private _currentOptions = _entity getVariable ["KH_var_remoteInteractionList", []];
_currentOptions pushBack [_name, _condition, _options];
_entity setVariable ["KH_var_remoteInteractionList", _currentOptions];