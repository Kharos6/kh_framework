params [["_entity", objNull, [objNull]], ["_respawn", false, [true]]];

[
    [_entity],
    {
        params ["_entity"];

        {
            _x params ["_typeInclude", "_typeExclude", "_function", "_id"];

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

            [_entity] call _function;
        } forEach KH_var_entityInitializations;
    },
    [true, "GLOBAL"] select _respawn,
    "-2",
    false
] call KH_fnc_execute;