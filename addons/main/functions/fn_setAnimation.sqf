params [["_unit", objNull, [objNull]], ["_animation", [], [[]]], ["_allowVehicle", false, [true]], ["_allowUnconscious", false, [true]]];

if (_animation isEqualTo []) exitWith {
    false;
};

private _finalAnimation = "";

private _result = if !((_animation select 0) isEqualType "") then {
    private _valid = true;
    private _switchUsed = false;

    {
        _x params [["_animationType", "ACTION_PLAY_NOW", [""]], ["_currentAnimation", "", ["", []]], ["_validate", true, [true]]];

        if ("SWITCH" in _animationType) then {
            _switchUsed = true;
        };

        if ("ACTION" in _animationType) then {
            private _moves = getText ((configOf _unit) >> "moves");
            private _action = getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions");
            _finalAnimation = toLowerANSI (getText (configFile >> _moves >> "Actions" >> _action >> _currentAnimation));

            if ((_finalAnimation isEqualTo "") && (((getArray (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) param [0, ""]) isEqualTo "")) then {
                if _validate then {
                    _valid = false;
                };

                break;
            };
        }
        else {
            _finalAnimation = if (_currentAnimation isEqualType "") then {
                toLowerANSI _currentAnimation;
            }
            else {
                toLowerANSI (_currentAnimation select 0);
            };

            if (_animationType isEqualTo "GESTURE_SWITCH") then {
                if ((getText (configFile >> (getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures")) >> "states" >> _finalAnimation)) isEqualTo "") then {
                    if _validate then {
                        _valid = false;
                    };

                    break;
                };
            }
            else {
                if ((getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> _finalAnimation)) isEqualTo "") then {
                    if _validate then {
                        _valid = false;
                    };

                    break;
                };
            };
        };
    } forEach _animation;

    if (!_valid || (!_allowVehicle && !(isNull (objectParent _unit))) || (!_allowUnconscious && (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")))) exitWith {
        false;
    };

    [
        [
            [_unit, _animation],
            {
                params ["_unit", "_animation"];
                
                {
                    _x params [["_animationType", "ACTION_PLAY_NOW", [""]], ["_currentAnimation", "", ["", []]]];

                    if (_currentAnimation isEqualTo "KH_GestureNone") then {
                        private _gestureId = generateUid;
                        _unit setVariable ["KH_var_gestureStopper", _gestureId];

                        [
                            [_unit, _gestureId],
                            {
                                params ["_unit", "_gestureId"];

                                if ((_unit getVariable ["KH_var_gestureStopper", ""]) isEqualTo _gestureId) then {
                                    _unit switchGesture [""];
                                };
                            },
                            true,
                            "0.5",
                            false
                        ] call KH_fnc_execute;
                    };

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
                } forEach _animation;
            }
        ],
        if (_switchUsed && !isUIContext) then {
            "KH_fnc_uiContextCall";
        }
        else {
            {
                (_this select 0) call (_this select 1);
            };
        },
        true,
        false
    ] call KH_fnc_execute;

    _valid;
}
else {
    if ((!_allowVehicle && !(isNull (objectParent _unit))) || (!_allowUnconscious && (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")))) exitWith {
        false;
    };

    _animation params [["_animationType", "ACTION_PLAY_NOW", [""]], ["_currentAnimation", "", ["", []]], ["_validate", true, [true]]];

    private _valid = if ("ACTION" in _animationType) then {
        private _moves = getText ((configOf _unit) >> "moves");
        private _action = getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions");
        _finalAnimation = toLowerANSI (getText (configFile >> _moves >> "Actions" >> _action >> _currentAnimation));
        (_finalAnimation isNotEqualTo "") || (((getArray (configFile >> _moves >> "Actions" >> _action >> _currentAnimation)) param [0, ""]) isNotEqualTo "");
    }
    else {
        _finalAnimation = if (_currentAnimation isEqualType "") then {
            toLowerANSI _currentAnimation;
        }
        else {
            toLowerANSI (_currentAnimation select 0);
        };

        if (_animationType isEqualTo "GESTURE_SWITCH") then {
            ((getText (configFile >> (getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures")) >> "states" >> _finalAnimation)) isNotEqualTo "");
        }
        else {
            ((getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> _finalAnimation)) isNotEqualTo "");
        };
    };

    if (!_valid && _validate) exitWith {
        false;
    };

    if (_currentAnimation isEqualTo "KH_GestureNone") then {
        private _gestureId = generateUid;
        _unit setVariable ["KH_var_gestureStopper", _gestureId];

        [
            [_unit, _gestureId],
            {
                params ["_unit", "_gestureId"];

                if ((_unit getVariable ["KH_var_gestureStopper", ""]) isEqualTo _gestureId) then {
                    _unit switchGesture [""];
                };
            },
            true,
            "0.5",
            false
        ] call KH_fnc_execute;
    };

    switch _animationType do {
        case "MOVE_PLAY": {
            _unit playMove _currentAnimation;
        };

        case "MOVE_PLAY_NOW": {
            _unit playMoveNow _currentAnimation;
        };

        case "MOVE_SWITCH": {
            if (_unit isEqualTo KH_var_playerUnit) then {
                [
                    [_unit, _currentAnimation],
                    {
                        params ["_unit", "_currentAnimation"];
                        _unit switchMove _currentAnimation;
                    }
                ] call KH_fnc_uiContextCall;
            }
            else {
                _unit switchMove _currentAnimation;
            };
        };

        case "MOVE_SWITCH_GLOBAL": {
            if (_unit isEqualTo KH_var_playerUnit) then {
                [
                    [_unit, _currentAnimation],
                    {
                        params ["_unit", "_currentAnimation"];
                        _unit switchMove _currentAnimation;
                    }
                ] call KH_fnc_uiContextCall;
            }
            else {
                _unit switchMove _currentAnimation;
            };

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
            if (_unit isEqualTo KH_var_playerUnit) then {
                [
                    [_unit, _currentAnimation],
                    {
                        params ["_unit", "_currentAnimation"];
                        _unit switchGesture _currentAnimation;
                    }
                ] call KH_fnc_uiContextCall;
            }
            else {
                _unit switchGesture _currentAnimation;
            };
        };

        case "ACTION_PLAY": {
            _unit playAction _currentAnimation;
        };

        case "ACTION_PLAY_NOW": {
            _unit playActionNow _currentAnimation;
        };

        case "ACTION_SWITCH": {
            if (_unit isEqualTo KH_var_playerUnit) then {
                [
                    [_unit, _currentAnimation],
                    {
                        params ["_unit", "_currentAnimation"];
                        _unit switchAction _currentAnimation;
                    }
                ] call KH_fnc_uiContextCall;
            }
            else {
                _unit switchAction _currentAnimation;
            };
        };
    };

    true;
};

if (_result && (_finalAnimation isNotEqualTo "") && (_unit isEqualTo KH_var_playerUnit)) then {
    private _interruptableId = generateUid;
    _unit setVariable ["KH_var_interruptableAnimationId", _interruptableId];
    _unit setVariable ["KH_var_currentInterruptableAnimation", ""];

    [
        [_unit, _finalAnimation, _interruptableId, false, 0],
        {
            params ["_unit", "_finalAnimation", "_interruptableId", "_animationReached", "_delay"];
            private _animationState = animationState _unit;
            private _currentInterruptableAnimation = _unit getVariable ["KH_var_currentInterruptableAnimation", ""];

            if (
                ((_unit getVariable ["KH_var_interruptableAnimationId", ""]) isNotEqualTo _interruptableId) || 
                (_unit isNotEqualTo KH_var_playerUnit) ||
                !(isNull (objectParent _unit)) ||
                (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")) ||
                (_animationReached && (_animationState isNotEqualTo _finalAnimation) && (_currentInterruptableAnimation isEqualTo "") && ((flatten (getAnimationsQueue _unit)) isEqualTo []))
               ) exitWith {
                _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                [_handlerId] call KH_fnc_removeHandler;
            };

            if _animationReached then {
                if (diag_tickTime > _delay) then {
                    private _moveType = [[_unit] call KH_fnc_getUnitMoveType, (["ARMA_STRING", [true, true, false]] call KH_fnc_getPlayerMoveInput) select 0] joinString "";

                    if (_moveType isEqualTo "STOP") then {
                        if ((_animationState isNotEqualTo _finalAnimation) && (_currentInterruptableAnimation isNotEqualTo "")) then {
                            _unit playActionNow "Stop";
                            _unit setVariable ["KH_var_interruptableAnimationId", ""];
                            _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                            [_handlerId] call KH_fnc_removeHandler;
                        };
                    }
                    else {
                        private _moves = getText ((configOf _unit) >> "moves");
                        _unit setVariable ["KH_var_currentInterruptableAnimation", getText (configFile >> _moves >> "Actions" >> (getText (configFile >> _moves >> "states" >> _animationState >> "actions")) >> _moveType)];
                        _unit playActionNow _moveType;
                    };
                };
            }
            else {
                _this set [3, _animationState isEqualTo _finalAnimation];
                _this set [4, diag_tickTime + (((1 / ((getNumber (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> _finalAnimation >> "interpolationSpeed")) max 0.001)) * 0.5) max 0.25)];
            };
        },
        true,
        [
            0,
            false,
            false,
            {},
            false,
            true
        ],
        false
    ] call KH_fnc_execute;
};

_result;