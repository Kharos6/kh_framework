params [["_unit", objNull, [objNull]], ["_action", "", [""]], ["_move", true, [true]], ["_config", true, [true]]];

if _move then {
    if _config then {
        private _statesConfig = _unit getVariable ["KH_var_movesStatesConfig", configNull];
        _statesConfig >> (getText ((_unit getVariable ["KH_var_movesActionsConfig", configNull]) >> (getText (_statesConfig >> (animationState _unit) >> "actions")) >> _action));
    }
    else {
        getText ((_unit getVariable ["KH_var_movesActionsConfig", configNull]) >> (getText ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> (animationState _unit) >> "actions")) >> _action);
    };
}
else {
    if _config then {
        (_unit getVariable ["KH_var_gesturesStatesConfig", configNull]) >> ((getArray ((_unit getVariable ["KH_var_movesActionsConfig", configNull]) >> (getText ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> (animationState _unit) >> "actions")) >> _action)) select 0);
    }
    else {
        (getArray ((_unit getVariable ["KH_var_movesActionsConfig", configNull]) >> (getText ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> (animationState _unit) >> "actions")) >> _action)) select 0;
    };
};