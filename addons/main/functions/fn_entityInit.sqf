params [["_typeInclude", [], [[]]], ["_typeExclude", [], [[]]], "_args", ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
private _initId = generateUid;
private _blockerId = generateUid;
missionNamespace setVariable [_initId, true];
KH_var_entityInitializations pushBack [_typeInclude, _typeExclude, _args, _function, _initId, _blockerId];

if _applyRetroactively then {
    {
        if (_x getVariable [_blockerId, false]) then {
            continue;
        };

        _x setVariable [_blockerId, true];
        [_x, false, objNull] call _function;
    } forEach ((allMissionObjects "") select {
        private _entity = _x;
        private _continue = true;

        if (_typeExclude isNotEqualTo []) then {
            {
                if (_entity isKindOf _x) then {
                    _continue = false;
                    break;
                };
            } forEach _typeExclude;
        };
        
        if _continue then {
            if (_typeInclude isNotEqualTo []) then {
                _continue = false;
                
                {
                    if (_entity isKindOf _x) then {
                        _continue = true;
                        break;
                    };
                } forEach _typeInclude;
            };
        };

        _continue;
    });
};

[missionNamespace, _initId, clientOwner];