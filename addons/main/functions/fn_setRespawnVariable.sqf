params [["_object", objNull, [objNull]], ["_variableName", "", [""]], "_value", ["_public", false, [true, 0, []]], ["_set", true, [true]]];
private _currentVariableHandler = _object getVariable "KH_var_respawnVariableHandler";

if (isNil "_currentVariableHandler") then {
    _object setVariable ["KH_var_respawnVariableHandler", createHashMap, true];

    [
        ["ENTITY", _object, "PERSISTENT"],
        "Respawn",
        [],
        {
            [
                _this,
                {
                    params ["_unit"];

                    {
                        _unit setVariable [_x, _y select 0, _y select 1];
                    } forEach (_unit getVariable "KH_var_respawnVariableHandler");
                },
                true,
                "-1",
                false
            ] call KH_fnc_execute;
        }
    ] call KH_fnc_addEventHandler;
};

if _set then {
    _currentVariableHandler set [_variableName, [_value, _public]];
}
else {
    _currentVariableHandler deleteAt _variableName;
};

_object setVariable ["KH_var_respawnVariableHandler", _currentVariableHandler, true];