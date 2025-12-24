params [["_typeInclude", [], [[]]], ["_typeExclude", [], [[]]], ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
KH_var_entityInitializations pushBack [_typeInclude, _typeExclude, _function];

if _applyRetroactively then {
    {
        [_x] call _function;
    } forEach (entities [_typeInclude, _typeExclude, true, false]);
};