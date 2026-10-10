params [["_typeInclude", [], [[]]], ["_typeExclude", [], [[]]], "_args", ["_function", {}, [{}]], ["_applyRetroactively", true, [true]]];
private _initId = generateUid;
private _blockerId = generateUid;
missionNamespace setVariable [_initId, true];
KH_var_entityInitializations pushBack [_typeInclude, _typeExclude, _args, _function, _initId, _blockerId];
private _ammoConfig = configFile >> "CfgAmmo";

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

        private _type = typeOf _entity;
        _continue && ((_type select [0, 1]) isNotEqualTo "#") && !(isClass (_ammoConfig >> _type));
    });
};

[missionNamespace, _initId, clientOwner];