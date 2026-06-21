params [["_entity", objNull, [objNull]], ["_respawn", false, [true]], ["_previousEntity", objNull, [objNull]]];

[
    [_entity, _respawn, _previousEntity],
    {
        params ["_entity", "_respawn", "_previousEntity"];

        {
            _x params ["_typeInclude", "_typeExclude", "_args", "_function", "_id", "_blockerId"];
            
            if (!_respawn && (_entity getVariable [_blockerId, false])) then {
                continue;
            };

            _entity setVariable [_blockerId, true];

            if !(missionNamespace getVariable _id) then {
                KH_var_entityInitializationsDeletions pushBack _id;
                continue;
            };

            private _continue = true;

            if (_typeExclude isNotEqualTo []) then {
                {
                    if (_entity isKindOf _x) then {
                        _continue = false;
                        break;
                    };
                } forEach _typeExclude;
            };
            
            if !_continue then {
                continue;
            };

            if (_typeInclude isNotEqualTo []) then {
                _continue = false;
                
                {
                    if (_entity isKindOf _x) then {
                        _continue = true;
                        break;
                    };
                } forEach _typeInclude;
            };

            if !_continue then {
                continue;
            };

            [_entity, _respawn, _previousEntity] call _function;
        } forEach KH_var_entityInitializations;
    },
    [true, "GLOBAL"] select _respawn,
    "-2",
    false
] call KH_fnc_execute;