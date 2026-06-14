params [["_unit", objNull, [objNull]], ["_animation", [], [[]]], ["_allowVehicle", false, [true]], ["_allowUnconscious", false, [true]]];

if ((_animation isEqualTo []) || (_unit getVariable ["KH_var_lockedAnimation", false])) exitWith {
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

if (
    _result && 
    (_finalAnimation isNotEqualTo "") && 
    (_unit isEqualTo KH_var_playerUnit) && 
    (isNull (objectParent _unit)) && 
    (isTouchingGround _unit) && 
    ((lifeState _unit) isNotEqualTo "INCAPACITATED") && 
    ((lifeState _unit) isNotEqualTo "UNCONSCIOUS")
   ) then {
    private _interruptableId = generateUid;
    _unit setVariable ["KH_var_interruptableAnimationId", _interruptableId];
    _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
    _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];

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
                !(isTouchingGround _unit) ||
                (_animationReached && (_animationState isNotEqualTo _finalAnimation) && (_currentInterruptableAnimation isEqualTo "") && ((flatten (getAnimationsQueue _unit)) isEqualTo []))
               ) exitWith {
                _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];
                [_handlerId] call KH_fnc_removeHandler;
            };

            if _animationReached then {
                if (diag_tickTime > _delay) then {
                    private _switchPrimary = (inputAction "SwitchPrimary") isNotEqualTo 0;
                    private _switchSecondary = (inputAction "SwitchHandgun") isNotEqualTo 0;
                    private _switchTertiary = (inputAction "SwitchSecondary") isNotEqualTo 0;
                    private _switchBinocular = (inputAction "binocular") isNotEqualTo 0;
                    private _raiseWeapon = (inputAction "toggleRaiseWeapon") isNotEqualTo 0;
                    private _throw = (inputAction "throw") isNotEqualTo 0;
                    private _reload = (inputAction "reloadMagazine") isNotEqualTo 0;
                    private _prone = (inputAction "MoveDown") isNotEqualTo 0;
                    private _crouch = (inputAction "MoveUp") isNotEqualTo 0;
                    private _fire = ((inputAction "defaultAction") isNotEqualTo 0) && !(_unit getVariable ["KH_var_inMeleeState", false]) && ((_unit getVariable ["KH_var_meleeMode", ""]) isEqualTo "");

                    if ((_currentInterruptableAnimation isNotEqualTo "") && (_switchPrimary || _switchSecondary || _switchTertiary || _switchBinocular || _raiseWeapon || _throw || _reload || _prone || _crouch || _fire)) exitWith {
                        _unit playActionNow "Stop";
                        _unit setVariable ["KH_var_lockedAnimation", true];
                        private _moves = getText ((configOf _unit) >> "moves");

                        if (_raiseWeapon || _prone || _crouch) then {
                            [
                                [
                                    _unit, 
                                    toLowerANSI (getText (configFile >> _moves >> "Actions" >> (getText (configFile >> _moves >> "states" >> _animationState >> "actions")) >> "stop")),
                                    [["down", "up"] select _crouch, "Stand"] select _raiseWeapon
                                ],
                                {
                                    params ["_unit", "_state", "_newState"];

                                    if (
                                        (_unit isNotEqualTo KH_var_playerUnit) ||
                                        !(isNull (objectParent _unit)) ||
                                        (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")) ||
                                        !(isTouchingGround _unit)
                                    ) exitWith {
                                        _unit setVariable ["KH_var_lockedAnimation", false];
                                        [_handlerId] call KH_fnc_removeHandler;
                                    };

                                    private _queue = flatten (getAnimationsQueue _unit);

                                    if (((animationState _unit) isEqualTo _state) || !(_state in _queue) || (_queue isEqualTo [])) then {
                                        _unit playActionNow _newState;
                                        _unit setVariable ["KH_var_lockedAnimation", false];
                                        [_handlerId] call KH_fnc_removeHandler;
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
                        }
                        else {
                            [
                                [
                                    _unit, 
                                    toLowerANSI (getText (configFile >> _moves >> "Actions" >> (getText (configFile >> _moves >> "states" >> _animationState >> "actions")) >> "stop")),
                                    _switchPrimary,
                                    _switchSecondary,
                                    _switchTertiary,
                                    _switchBinocular,
                                    _throw,
                                    _reload,
                                    _fire
                                ],
                                {
                                    params ["_unit", "_state", "_switchPrimary", "_switchSecondary", "_switchTertiary", "_switchBinocular", "_throw", "_reload", "_fire"];

                                    if (
                                        (_unit isNotEqualTo KH_var_playerUnit) ||
                                        !(isNull (objectParent _unit)) ||
                                        (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")) ||
                                        !(isTouchingGround _unit)
                                    ) exitWith {
                                        _unit setVariable ["KH_var_lockedAnimation", false];
                                        [_handlerId] call KH_fnc_removeHandler;
                                    };

                                    private _queue = flatten (getAnimationsQueue _unit);

                                    if (((animationState _unit) isEqualTo _state) || !(_state in _queue) || (_queue isEqualTo [])) then {
                                        switch true do {
                                            case _switchPrimary: {
                                                _unit action ["SwitchWeapon", _unit, _unit, ((_unit weaponsInfo [primaryWeapon _unit, false]) param [0, []]) param [0, -1]];
                                            };
                                            
                                            case _switchSecondary: {
                                                _unit action ["SwitchWeapon", _unit, _unit, ((_unit weaponsInfo [handgunWeapon _unit, false]) param [0, []]) param [0, -1]];
                                            };

                                            case _switchTertiary: {
                                                _unit action ["SwitchWeapon", _unit, _unit, ((_unit weaponsInfo [secondaryWeapon _unit, false]) param [0, []]) param [0, -1]];
                                            };
                                            
                                            case _switchBinocular: {
                                                _unit action ["SwitchWeapon", _unit, _unit, ((_unit weaponsInfo [binocular _unit, false]) param [0, []]) param [0, -1]];
                                            };
                                            
                                            case _throw: {
                                                _unit action ["UseWeapon", _unit, _unit, ((_unit weaponsInfo [(currentThrowable _unit) select 0, false]) param [0, []]) param [0, -1]];
                                            };

                                            case _reload: {
                                                reload _unit;
                                            };
                                        
                                            case _fire: {
                                                _unit action ["UseWeapon", _unit, _unit, ((_unit weaponsInfo [currentMuzzle _unit, false]) param [0, []]) param [0, -1]];
                                            };
                                        };

                                        _unit setVariable ["KH_var_lockedAnimation", false];
                                        [_handlerId] call KH_fnc_removeHandler;
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

                        _unit setVariable ["KH_var_interruptableAnimationId", ""];
                        _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                        _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];
                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    private _moveType = [[_unit] call KH_fnc_getUnitMoveType, (["ARMA_STRING", [true, true, false]] call KH_fnc_getPlayerMoveInput) select 0] joinString "";

                    if (_moveType isEqualTo "STOP") then {
                        if ((_animationState isNotEqualTo _finalAnimation) && (_currentInterruptableAnimation isNotEqualTo "")) then {
                            _unit playActionNow "Stop";
                            _unit setVariable ["KH_var_interruptableAnimationId", ""];
                            _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                            _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];
                            [_handlerId] call KH_fnc_removeHandler;
                        };
                    }
                    else {
                        private _moves = getText ((configOf _unit) >> "moves");
                        private _move = toLowerANSI (getText (configFile >> _moves >> "Actions" >> (getText (configFile >> _moves >> "states" >> _animationState >> "actions")) >> _moveType));

                        if ((_unit getVariable ["KH_var_currentInterruptableAnimation", ""]) isNotEqualTo _move) then {
                            _unit setVariable ["KH_var_currentInterruptableAnimation", _move];
                            _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];
                            _unit playActionNow _moveType;
                        };

                        if (_animationState isEqualTo _currentInterruptableAnimation) then {
                            _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", (_unit getVariable ["KH_var_currentInterruptableAnimationPeriod", 0]) + _totalDelta];

                            if ((_unit getVariable ["KH_var_currentInterruptableAnimationPeriod", 0]) >= (_unit getUnitMovesInfo 2)) then {
                                _unit setVariable ["KH_var_interruptableAnimationId", ""];
                                _unit setVariable ["KH_var_currentInterruptableAnimation", ""];
                                _unit setVariable ["KH_var_currentInterruptableAnimationPeriod", 0];
                                [_handlerId] call KH_fnc_removeHandler;
                            };
                        };
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