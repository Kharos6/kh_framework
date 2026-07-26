params [["_object", objNull, [objNull]], ["_variableName", "", [""]], "_value", ["_public", false, [true, 0, []]], ["_set", true, [true]]];
private _currentVariableHandler = _object getVariable "KH_var_respawnVariableHandler";

if (isNil "_currentVariableHandler") then {
    _currentVariableHandler = createHashMap;

    [
        ["ENTITY", _object, "PERSISTENT"],
        "Respawn",
        [],
        {
            execute [
                _this,
                {
                    params ["_unit"];

                    {
                        _unit setVariable [_x, _y select 0, _y select 1];
                    } forEach (_unit getVariable ["KH_var_respawnVariableHandler", createHashMap]);
                },
                true,
                "-2",
                false
            ];
        }
    ] call KH_fnc_addEventHandler;

    _object setVariable ["KH_var_respawnVariableHandler", _currentVariableHandler];
};

if _set then {
    _currentVariableHandler set [_variableName, [_value, _public]];
}
else {
    _currentVariableHandler deleteAt _variableName;
};

nil;