params [["_typeInclude", [], [[]]], ["_typeExclude", [], [[]]], ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
private _initId = generateUid;
missionNamespace setVariable [_initId, true];
KH_var_entityInitializations pushBack [_typeInclude, _typeExclude, _function, _initId];

if _applyRetroactively then {
    {
        [_x] call _function;
    } forEach (entities [_typeInclude, _typeExclude, true, false]);
};

[missionNamespace, _initId, clientOwner];