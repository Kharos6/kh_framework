params [["_unit", objNull, [objNull]], ["_animation", [], [[]]], ["_allowVehicle", false, [true]], ["_force", false, [true]]];

if !_force then {
    if (_animation isNotEqualTo []) then {
        if !((_animation select 0) isEqualType "") then {
            private _currentAnimationSequenceFrame = diag_frameNo;
            _unit setVariable ["KH_var_currentAnimationSequenceFrame", _currentAnimationSequenceFrame];
            private _totalDelay = 0;

            {
                _x params ["_delay", "_animationType", "_currentAnimation"];
                _totalDelay = _totalDelay + _delay;

                if (_totalDelay isEqualTo 0) then {
                    if (!_allowVehicle && !(isNull (objectParent _unit))) then {
                        continue;
                    }
                    else {
                        switch _animationType do {
                            case "MOVE_PLAY": {
                                _unit playMove _currentAnimation;
                            };

                            case "MOVE_PLAY_NOW": {
                                _unit playMoveNow _currentAnimation;
                            };

                            case "MOVE_SWITCH": {
                                _unit switchMove _currentAnimation;
                            };

                            case "MOVE_SWITCH_GLOBAL": {
                                _unit switchMove _currentAnimation;

                                [
                                    [_unit, _currentAnimation],
                                    {
                                        params ["_unit", "_currentAnimation"];
                                        _unit switchMove _currentAnimation;
                                    },
                                    -clientOwner,
                                    true,
                                    false
                                ] call KH_fnc_execute;
                            };

                            case "ACTION": {
                                _unit playAction _currentAnimation;
                            };

                            case "ACTION_PLAY_NOW": {
                                _unit playActionNow _currentAnimation;
                            };

                            case "ACTION_SWITCH": {
                                _unit switchAction _currentAnimation;
                            };
                        };

                        continue;
                    };
                };

                [
                    [_unit, _allowVehicle, _currentAnimationSequenceFrame, _animationType, _currentAnimation],
                    {
                        params ["_unit", "_allowVehicle", "_currentAnimationSequenceFrame", "_animationType", "_currentAnimation"];
                        
                        if ((_unit getVariable ["KH_var_currentAnimationSequenceFrame", 0]) isEqualTo _currentAnimationSequenceFrame) then {
                            if (!_allowVehicle && !(isNull (objectParent _unit))) exitWith {};

                            switch _animationType do {
                                case "MOVE_PLAY": {
                                    _unit playMove _currentAnimation;
                                };

                                case "MOVE_PLAY_NOW": {
                                    _unit playMoveNow _currentAnimation;
                                };

                                case "MOVE_SWITCH": {
                                    _unit switchMove _currentAnimation;
                                };

                                case "MOVE_SWITCH_GLOBAL": {
                                    _unit switchMove _currentAnimation;

                                    [
                                        [_unit, _currentAnimation],
                                        {
                                            params ["_unit", "_currentAnimation"];
                                            _unit switchMove _currentAnimation;
                                        },
                                        -clientOwner,
                                        true,
                                        false
                                    ] call KH_fnc_execute;
                                };

                                case "ACTION": {
                                    _unit playAction _currentAnimation;
                                };

                                case "ACTION_PLAY_NOW": {
                                    _unit playActionNow _currentAnimation;
                                };

                                case "ACTION_SWITCH": {
                                    _unit switchAction _currentAnimation;
                                };
                            };
                        };
                    },
                    true,
                    str _totalDelay,
                    false
                ] call KH_fnc_execute;
            } forEach _animation;
        }
        else {
            if (!_allowVehicle && !(isNull (objectParent _unit))) exitWith {};
            _animation params ["_animationType", "_currentAnimation"];

            switch _animationType do {
                case "MOVE_PLAY": {
                    _unit playMove _currentAnimation;
                };

                case "MOVE_PLAY_NOW": {
                    _unit playMoveNow _currentAnimation;
                };

                case "MOVE_SWITCH": {
                    _unit switchMove _currentAnimation;
                };

                case "MOVE_SWITCH_GLOBAL": {
                    _unit switchMove _currentAnimation;

                    [
                        [_unit, _currentAnimation],
                        {
                            params ["_unit", "_currentAnimation"];
                            _unit switchMove _currentAnimation;
                        },
                        -clientOwner,
                        true,
                        false
                    ] call KH_fnc_execute;
                };

                case "ACTION": {
                    _unit playAction _currentAnimation;
                };

                case "ACTION_PLAY_NOW": {
                    _unit playActionNow _currentAnimation;
                };

                case "ACTION_SWITCH": {
                    _unit switchAction _currentAnimation;
                };
            };
        };
    };
}
else {
    private _position = getPosATL _unit;
    private _direction = getDir _unit;
    private _dummyVehicle = createVehicleLocal ["B_Quadbike_01_F", _position, [], 0, "CAN_COLLIDE"];
    _dummyVehicle hideObject true;
    _dummyVehicle setPhysicsCollisionFlag false;
    _unit moveInAny _dummyVehicle;

    [
        [_unit, _dummyVehicle, _position, _direction, _animation],
        {
            params ["_unit", "_dummyVehicle", "_position", "_direction", "_animation"];
            unassignVehicle _unit;
            _unit action ["Eject", objectParent _unit];
            _unit setPosATL _position;
            _unit setDir _direction;
            deleteVehicle _dummyVehicle;

            if (_animation isNotEqualTo []) then {
                if !((_animation select 0) isEqualType "") then {
                    private _currentAnimationSequenceFrame = diag_frameNo;
                    _unit setVariable ["KH_var_currentAnimationSequenceFrame", _currentAnimationSequenceFrame];
                    private _totalDelay = 0;

                    {
                        _x params ["_delay", "_animationType", "_currentAnimation"];
                        _totalDelay = _totalDelay + _delay;

                        if (_totalDelay isEqualTo 0) then {
                            if (!_allowVehicle && !(isNull (objectParent _unit)) && ((objectParent _unit) isNotEqualTo _dummyVehicle)) then {
                                continue;
                            }
                            else {
                                switch _animationType do {
                                    case "MOVE_PLAY": {
                                        _unit playMove _currentAnimation;
                                    };

                                    case "MOVE_PLAY_NOW": {
                                        _unit playMoveNow _currentAnimation;
                                    };

                                    case "MOVE_SWITCH": {
                                        _unit switchMove _currentAnimation;
                                    };

                                    case "MOVE_SWITCH_GLOBAL": {
                                        _unit switchMove _currentAnimation;

                                        [
                                            [_unit, _currentAnimation],
                                            {
                                                params ["_unit", "_currentAnimation"];
                                                _unit switchMove _currentAnimation;
                                            },
                                            -clientOwner,
                                            true,
                                            false
                                        ] call KH_fnc_execute;
                                    };

                                    case "ACTION": {
                                        _unit playAction _currentAnimation;
                                    };

                                    case "ACTION_PLAY_NOW": {
                                        _unit playActionNow _currentAnimation;
                                    };

                                    case "ACTION_SWITCH": {
                                        _unit switchAction _currentAnimation;
                                    };
                                };

                                continue;
                            };
                        };

                        [
                            [_unit, _allowVehicle, _currentAnimationSequenceFrame, _animationType, _currentAnimation],
                            {
                                params ["_unit", "_allowVehicle", "_currentAnimationSequenceFrame", "_animationType", "_currentAnimation"];

                                if ((_unit getVariable ["KH_var_currentAnimationSequenceFrame", 0]) isEqualTo _currentAnimationSequenceFrame) then {
                                    if (!_allowVehicle && !(isNull (objectParent _unit))) exitWith {};

                                    switch _animationType do {
                                        case "MOVE_PLAY": {
                                            _unit playMove _currentAnimation;
                                        };

                                        case "MOVE_PLAY_NOW": {
                                            _unit playMoveNow _currentAnimation;
                                        };

                                        case "MOVE_SWITCH": {
                                            _unit switchMove _currentAnimation;
                                        };

                                        case "MOVE_SWITCH_GLOBAL": {
                                            _unit switchMove _currentAnimation;

                                            [
                                                [_unit, _currentAnimation],
                                                {
                                                    params ["_unit", "_currentAnimation"];
                                                    _unit switchMove _currentAnimation;
                                                },
                                                -clientOwner,
                                                true,
                                                false
                                            ] call KH_fnc_execute;
                                        };

                                        case "ACTION": {
                                            _unit playAction _currentAnimation;
                                        };

                                        case "ACTION_PLAY_NOW": {
                                            _unit playActionNow _currentAnimation;
                                        };

                                        case "ACTION_SWITCH": {
                                            _unit switchAction _currentAnimation;
                                        };
                                    };
                                };
                            },
                            true,
                            str _totalDelay,
                            false
                        ] call KH_fnc_execute;
                    } forEach _animation;
                }
                else {
                    if (!_allowVehicle && !(isNull (objectParent _unit)) && ((objectParent _unit) isNotEqualTo _dummyVehicle)) exitWith {};
                    _animation params ["_animationType", "_currentAnimation"];

                    switch _animationType do {
                        case "MOVE_PLAY": {
                            _unit playMove _currentAnimation;
                        };

                        case "MOVE_PLAY_NOW": {
                            _unit playMoveNow _currentAnimation;
                        };

                        case "MOVE_SWITCH": {
                            _unit switchMove _currentAnimation;
                        };

                        case "MOVE_SWITCH_GLOBAL": {
                            _unit switchMove _currentAnimation;

                            [
                                [_unit, _currentAnimation],
                                {
                                    params ["_unit", "_currentAnimation"];
                                    _unit switchMove _currentAnimation;
                                },
                                -clientOwner,
                                true,
                                false
                            ] call KH_fnc_execute;
                        };

                        case "ACTION": {
                            _unit playAction _currentAnimation;
                        };

                        case "ACTION_PLAY_NOW": {
                            _unit playActionNow _currentAnimation;
                        };

                        case "ACTION_SWITCH": {
                            _unit switchAction _currentAnimation;
                        };
                    };
                };
            };
        },
        true,
        "-1",
        false
    ] call KH_fnc_execute;
};

nil;