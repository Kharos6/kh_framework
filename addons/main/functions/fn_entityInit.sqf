params [["_typeInclude", [], [[]]], ["_typeExclude", [], [[]]], ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
private _initId = generateUid;
private _blockerId = generateUid;
missionNamespace setVariable [_initId, true];
KH_var_entityInitializations pushBack [_typeInclude, _typeExclude, _function, _initId, _blockerId];

if _applyRetroactively then {
    {
        if (_x getVariable [_blockerId, false]) then {
            continue;
        };

        _x setVariable [_blockerId, true];
        [_x] call _function;
    } forEach (entities [_typeInclude, _typeExclude, true, false]);
};

[missionNamespace, _initId, clientOwner];