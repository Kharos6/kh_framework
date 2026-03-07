params [["_unit", objNull, [objNull]], ["_animation", [], [[]]], ["_allowVehicle", false, [true]], ["_allowUnconscious", false, [true]]];

if (_animation isEqualTo []) exitWith {
    false;
};

if !((_animation select 0) isEqualType "") then {
    private _currentAnimationSequenceId = generateUid;
    _unit setVariable ["KH_var_currentAnimationSequenceId", _currentAnimationSequenceId];
    private _totalDelay = 0;
    private _valid = true;

    {
        _x params [["_delay", 0, [0]], ["_animationType", "ACTION_PLAY_NOW", [""]], ["_currentAnimation", "", ["", []]], ["_validate", true, [true]]];
        _totalDelay = _totalDelay + _delay;

        if _validate then {
            if ("ACTION" in _animationType) then {
                private _moves = getText ((configOf _unit) >> "moves");
                private _action = getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions");

                if ((getText (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) isEqualTo "") && (((getArray (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) param [0, ""]) isEqualTo "") then {
                    _valid = false;
                    break;
                };
            }
            else {
                if (_animationType isEqualTo "GESTURE_SWITCH") then {
                    if ((getText (configFile >> (getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures")) >> "states" >> _currentAnimation)) isEqualTo "") then {
                        _valid = false;
                        break;
                    };
                }
                else {
                    if ((getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> _currentAnimation)) isEqualTo "") then {
                        _valid = false;
                        break;
                    };
                };
            };
        };

        if (_totalDelay isEqualTo 0) then {
            if ((!_allowVehicle && !(isNull (objectParent _unit))) || (!_allowUnconscious && (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")))) then {
                _valid = false;
                break;
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

                    case "GESTURE_SWITCH": {
                        _unit switchGesture _currentAnimation;
                    };

                    case "ACTION_PLAY": {
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
            [_unit, _allowVehicle, _currentAnimationSequenceId, _animationType, _currentAnimation],
            {
                params ["_unit", "_allowVehicle", "_currentAnimationSequenceId", "_animationType", "_currentAnimation"];
                
                if ((_unit getVariable ["KH_var_currentAnimationSequenceId", ""]) isEqualTo _currentAnimationSequenceId) then {
                    if ((!_allowVehicle && !(isNull (objectParent _unit))) || (!_allowUnconscious && (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")))) exitWith {};

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

                        case "GESTURE_SWITCH": {
                            _unit switchGesture _currentAnimation;
                        };
                        
                        case "ACTION_PLAY": {
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

    _valid;
}
else {
    if ((!_allowVehicle && !(isNull (objectParent _unit))) || (!_allowUnconscious && (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")))) exitWith {
        false;
    };

    _animation params [["_animationType", "ACTION_PLAY_NOW", [""]], ["_currentAnimation", "", ["", []]], ["_validate", true, [true]]];

    private _valid = if _validate then {
        if ("ACTION" in _animationType) then {
            private _moves = getText ((configOf _unit) >> "moves");
            private _action = getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions");
            ((getText (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) isNotEqualTo "") || (((getArray (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) param [0, ""]) isNotEqualTo "");
        }
        else {
            if (_animationType isEqualTo "GESTURE_SWITCH") then {
                ((getText (configFile >> (getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures")) >> "states" >> _currentAnimation)) isNotEqualTo "");
            }
            else {
                ((getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> _currentAnimation)) isNotEqualTo "");
            };
        };
    }
    else {
        true;
    };

    if _valid then {
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

            case "GESTURE_SWITCH": {
                _unit switchGesture _currentAnimation;
            };

            case "ACTION_PLAY": {
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

    _valid;
};