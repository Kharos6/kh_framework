params [["_unit", objNull, [objNull]], ["_lock", true, [true]], ["_rotation", [], [[]]]];

if ((_unit getVariable ["KH_var_unitRotationLockHandler", []]) isNotEqualTo []) then {
    [_unit getVariable ["KH_var_unitRotationLockHandler", []]] call KH_fnc_removeHandler;
    _unit setVariable ["KH_var_unitRotationLockHandler", []];
};

if _lock then {
    _unit setVariable [
        "KH_var_unitRotationLockHandler",
        [
            [
                _unit,
                if (_rotation isEqualTo []) then {
                    [vectorDir _unit, vectorUp _unit];
                }
                else {
                    if (_rotation isEqualTypeAll 0) then {
                        eulerToVector _rotation;
                    }
                    else {
                        _rotation;
                    };
                }
            ],
            {
                params ["_unit", "_vectorDirAndUp"];
                _unit setVectorDirAndUp _vectorDirAndUp;
            },
            true,
            0,
            false
        ] call KH_fnc_execute
    ];
};

nil;