params [["_type", "", [""]], ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
KH_var_entityInitializations pushBack [_type, _function];

if _applyRetroactively then {
    {
        [_x] call _function;
    } forEach (KH_var_allEntities select {_x isKindOf _type;});
};