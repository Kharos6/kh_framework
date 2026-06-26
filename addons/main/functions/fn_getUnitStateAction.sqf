params [["_unit", objNull, [objNull]], ["_state", "", [""]], ["_config", true, [true]]];

if _config then {
    (_unit getVariable ["KH_var_movesActionsConfig", configNull]) >> (getText ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> _state >> "actions"));
}
else {
    getText ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> _state >> "actions");
};