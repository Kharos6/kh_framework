params [["_unit", objNull, [objNull]], ["_action", "", ["", []]]];

if (
    !(alive _unit) ||
    !(isNull (objectParent _unit)) || 
    !(isNull (attachedTo _unit)) ||
    ((lifeState _unit) isEqualTo "INCAPACITATED") || 
    ((lifeState _unit) isEqualTo "UNCONSCIOUS")
) exitWith {
    _unit setVariable ["KH_var_enteringMelee", false];
    _unit setVariable ["KH_var_inMeleeState", false, true];
    _unit setVariable ["KH_var_meleeMode", "", true];
    false;
};

if ((currentWeapon _unit) isNotEqualTo "") then {
    private _currentWeapon = currentWeapon _unit;
    private _currentWeaponSlot = [_unit] call KH_fnc_getCurrentWeaponSlot;

    private _meleeAttachments = switch _currentWeaponSlot do {
        case "PRIMARY": {
            primaryWeaponItems _unit;
        };

        case "SECONDARY": {
            handgunItems _unit;
        };

        case "TERTIARY": {
            secondaryWeaponItems _unit;
        };

        default {
            [];  
        };
    };

    private _weaponConfig = if ((_meleeAttachments select {_x isNotEqualTo "";}) isEqualTo []) then {
        configFile >> "CfgWeapons" >> _currentWeapon;
    }
    else {
        private _finalConfig = configNull;
        
        {
            private _currentConfig = configFile >> "CfgWeapons" >> _x;

            if (((getText (_currentConfig >> "kh_meleeType")) isNotEqualTo "") && (((getArray (_currentConfig >> "kh_meleeModes")) isNotEqualTo []) || ((getArray (_currentConfig >> "kh_meleeModesGestures")) isNotEqualTo []))) then {
                _finalConfig = _currentConfig;
                break;
            };
        } forEach _meleeAttachments;

        if (isNull _finalConfig) then {
            configFile >> "CfgWeapons" >> _currentWeapon;
        }
        else {
            _finalConfig;
        };
    };

    if ((_unit getVariable ["KH_var_meleeType", ""]) isNotEqualTo (getText (_weaponConfig >> "kh_meleeType"))) then {
        _unit setVariable ["KH_var_meleeType", getText (_weaponConfig >> "kh_meleeType"), true];
    };

    private _meleeState = (getNumber (configFile >> "CfgWeapons" >> _currentWeapon >> "kh_meleeWeapon")) isEqualTo 1;
    _unit setVariable ["KH_var_meleeWeaponConfig", _weaponConfig];
    _unit setVariable ["KH_var_meleeMode", (getArray (_weaponConfig >> "kh_meleeModes")) param [0, ""], true];
    _unit setVariable ["KH_var_inMeleeState", _meleeState, true];

    if ((getText (_weaponConfig >> "kh_meleeActions")) isNotEqualTo "") then {
        _unit setVariable ["KH_var_enteringMelee", true];
        _unit setVariable ["KH_var_meleeWeaponSlot", _currentWeaponSlot];
        _unit action ["SwitchWeapon", _unit, _unit, 299];
    }
    else {
        if (((_unit getVariable ["KH_var_meleeWeaponSlot", ""]) isEqualTo _currentWeaponSlot) && !_meleeState) then {
            [
                _unit, 
                [
                    "ACTION_SWITCH", 
                    switch _currentWeaponSlot do {
                        case "PRIMARY": {
                            "PrimaryWeapon";
                        };

                        case "SECONDARY": {
                            "handgunOn";
                        };

                        case "TERTIARY": {
                            "SecondaryWeapon";
                        };

                        default {
                            "Civil";  
                        };
                    }, 
                    true
                ],
                false,
                false
            ] call KH_fnc_setAnimation;

            _unit setVariable ["KH_var_meleeWeaponSlot", ""];
        };
    };
}
else {
    if (_unit getVariable ["KH_var_enteringMelee", false]) then {
        [
            _unit, 
            [
                [0, "ACTION_SWITCH", [getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeActions"), "Transition"] joinString "", true], 
                [0, "ACTION_PLAY", getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeActions"), true]
            ], 
            false,
            false
        ] call KH_fnc_setAnimation;

        _unit setVariable ["KH_var_enteringMelee", false];
    };
};

if ((isSwitchingWeapon _unit) || (((_unit weaponState (currentWeapon _unit)) select 6) > 0)) exitWith {
    false;
};

private ["_subaction", "_modifier"];

if (_action isEqualType []) then {
    _subaction = _action param [1, false, [true, "", []]];
    _modifier = _action param [2, 0, [0, "", objNull]];
    _action = _action param [0, "", [""]];
};

if (_action isNotEqualTo "") then {
    switch _action do {
        case "ATTACK": {
            private _moves = getText ((configOf _unit) >> "moves");
            
            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                private _mode = _unit getVariable ["KH_var_meleeMode", ""];
                private _priority = [KH_var_meleeAiAttackAnimationTypePriority, KH_var_meleePlayerAttackAnimationTypePriority] select (isPlayer _unit);

                if (_priority isEqualTo 0) then {
                    if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> _mode)) isNotEqualTo "") then {
                        _unit setVariable ["KH_var_attackGestureIndex", -1];

                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", _mode, true], 
                                [0, "ACTION_PLAY", "KH_MeleeStop", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        private _gestureMode = (getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModesGestures")) param [(getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModes")) find _mode, []];
                        
                        if (_gestureMode isNotEqualTo []) then {
                            private _newIndex = (_unit getVariable ["KH_var_attackGestureIndex", -1]) + 1;
                            private _newMode = _gestureMode param [_newIndex, ""];

                            if (_newMode isEqualTo "") then {
                                _newIndex = 0;
                                _newMode = _gestureMode param [_newIndex, ""];
                            };

                            if (_newMode isNotEqualTo "") then {
                                private _gestureAction = (getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> (_gestureMode param [_newIndex, ""]))) param [0, ""];
                                
                                if (_gestureAction isNotEqualTo "") then {
                                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                                        _unit setVariable ["KH_var_attackGestureIndex", _newIndex];
                                        [_unit, ["ACTION_PLAY_NOW", _newMode, true], false, false] call KH_fnc_setAnimation;
                                    }
                                    else {
                                        _unit setVariable ["KH_var_attackGestureIndex", -1];
                                        false;
                                    };
                                };
                            }
                            else {
                                _unit setVariable ["KH_var_attackGestureIndex", -1];
                                false;
                            };
                        };
                    };
                }
                else {
                    private _gestureMode = (getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModesGestures")) param [(getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModes")) find _mode, []];
                    
                    if (_gestureMode isEqualTo []) then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", _mode, true], 
                                [0, "ACTION_PLAY", "KH_MeleeStop", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        private _newIndex = (_unit getVariable ["KH_var_attackGestureIndex", -1]) + 1;
                        private _newMode = _gestureMode param [_newIndex, ""];

                        if (_newMode isEqualTo "") then {
                            _newIndex = 0;
                            _newMode = _gestureMode param [_newIndex, ""];
                        };

                        if (_newMode isEqualTo "") then {
                            _unit setVariable ["KH_var_attackGestureIndex", -1];

                            [
                                _unit, 
                                [
                                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                    [0, "ACTION_PLAY_NOW", _mode, true], 
                                    [0, "ACTION_PLAY", "KH_MeleeStop", true]
                                ], 
                                false,
                                false
                            ] call KH_fnc_setAnimation;
                        }
                        else {
                            private _gestureAction = (getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> (_gestureMode param [_newIndex, ""]))) param [0, ""];

                            if (_gestureAction isEqualTo "") then {
                                _unit setVariable ["KH_var_attackGestureIndex", -1];

                                [
                                    _unit, 
                                    [
                                        [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                        [0, "ACTION_PLAY_NOW", _mode, true], 
                                        [0, "ACTION_PLAY", "KH_MeleeStop", true]
                                    ], 
                                    false,
                                    false
                                ] call KH_fnc_setAnimation;
                            }
                            else {
                                if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                                    _unit setVariable ["KH_var_attackGestureIndex", _newIndex];
                                    [_unit, ["ACTION_PLAY_NOW", _newMode, true], false, false] call KH_fnc_setAnimation;
                                }
                                else {
                                    _unit setVariable ["KH_var_attackGestureIndex", -1];
                                    false;
                                };
                            };
                        };
                    };
                };
            };
        };

        case "BLOCK_IN": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];
            private _moves = getText ((configOf _unit) >> "moves");

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                private _priority = [KH_var_meleeAiBlockAnimationTypePriority, KH_var_meleePlayerBlockAnimationTypePriority] select (isPlayer _unit);

                if (_priority isEqualTo 0) then {
                    if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockIn")) isNotEqualTo "") then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_MeleeBlockIn", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            [_unit, ["ACTION_PLAY_NOW", "KH_MeleeBlockGesture", true], false, false] call KH_fnc_setAnimation;
                        }
                        else {
                            false;
                        };
                    };
                }
                else {
                    if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockGesture")) select 0) isNotEqualTo "") then {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            [_unit, ["ACTION_PLAY_NOW", "KH_MeleeBlockGesture", true], false, false] call KH_fnc_setAnimation;
                        }
                        else {
                            false;
                        };
                    }
                    else {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_MeleeBlockIn", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    };
                };
            };
        };

        case "BLOCK_OUT": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];
            private _moves = getText ((configOf _unit) >> "moves");
            
            if ((getArray (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeBlockTiming")) isNotEqualTo []) then {
                [
                    _unit, 
                    [
                        [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                        [0, "ACTION_PLAY_NOW", "KH_MeleeBlockOut", true], 
                        [0, "ACTION_PLAY", "KH_MeleeStop", true]
                    ], 
                    false,
                    false
                ] call KH_fnc_setAnimation;
            };

            if ((getArray (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _unit) >> "kh_meleeBlockTiming")) isNotEqualTo []) then {   
                [_unit, ["ACTION_PLAY_NOW", "KH_GestureNone", true], false, false] call KH_fnc_setAnimation;
            }
            else {
                false;
            };
        };

        case "BLOCK_SUCCESS": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];
            private _moves = getText ((configOf _unit) >> "moves");
            private _priority = [KH_var_meleeAiBlockAnimationTypePriority, KH_var_meleePlayerBlockAnimationTypePriority] select (isPlayer _unit);

			if (_priority isEqualTo 0) then {
				if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccess")) isNotEqualTo "") then {
                    [
                        _unit, 
                        [
                            [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                            [0, "ACTION_PLAY_NOW", "KH_MeleeBlockSuccess", true], 
                            [0, "ACTION_PLAY", "KH_MeleeBlockIn", true]
                        ], 
                        false,
                        false
                    ] call KH_fnc_setAnimation;
				}
				else {
                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_MeleeBlockSuccessGesture", true],
                                [0, "ACTION_PLAY", "KH_MeleeBlockGesture", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        false;
                    };
				};
			}
			else {
				if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccessGesture")) select 0) isNotEqualTo "") then {
                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_MeleeBlockSuccessGesture", true],
                                [0, "ACTION_PLAY", "KH_MeleeBlockGesture", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        false;
                    };
				}
				else {
                    [
                        _unit, 
                        [
                            [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                            [0, "ACTION_PLAY_NOW", "KH_MeleeBlockSuccess", true], 
                            [0, "ACTION_PLAY", "KH_MeleeBlockIn", true]
                        ], 
                        false,
                        false
                    ] call KH_fnc_setAnimation;
				};
			};
        };

        case "BLOCK_FAILURE": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            [
                _unit, 
                [
                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                    [0, "ACTION_PLAY_NOW", "KH_MeleeBlockFailure", true], 
                    [0, "ACTION_PLAY", "KH_MeleeStop", true]
                ], 
                false,
                false
            ] call KH_fnc_setAnimation;
        };
        
        case "PARRY": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];
            private _moves = getText ((configOf _unit) >> "moves");

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                private _priority = [KH_var_meleeAiParryAnimationTypePriority, KH_var_meleePlayerParryAnimationTypePriority] select (isPlayer _unit);

                if (_priority isEqualTo 0) then {
                    if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParry")) isNotEqualTo "") then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_MeleeParry", true], 
                                [0, "ACTION_PLAY", "KH_MeleeStop", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    }
                    else {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            [_unit, ["ACTION_PLAY_NOW", "KH_MeleeParryGesture", true], false, false] call KH_fnc_setAnimation;
                        }
                        else {
                            false;
                        };
                    };
                }
                else {
                    if ((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParryGesture")) isNotEqualTo []) then {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            [_unit, ["ACTION_PLAY_NOW", "KH_MeleeParryGesture", true], false, false] call KH_fnc_setAnimation;
                        }
                        else {
                            false;
                        };
                    }
                    else {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_MeleeParry", true], 
                                [0, "ACTION_PLAY", "KH_MeleeStop", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    };
                };
            };
        };

        case "KICK": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                [
                    _unit, 
                    [
                        [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                        [0, "ACTION_PLAY_NOW", "KH_MeleeKick", true], 
                        [0, "ACTION_PLAY", "KH_MeleeStop", true]
                    ], 
                    false,
                    false
                ] call KH_fnc_setAnimation;
            }
            else {
                false;
            };
        };

        case "TACKLE": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                [
                    _unit, 
                    [
                        [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                        [0, "ACTION_PLAY_NOW", "KH_MeleeTackle", true], 
                        [0, "ACTION_PLAY", "KH_MeleeStop", true]
                    ], 
                    false,
                    false
                ] call KH_fnc_setAnimation;
            }
            else {
                false;
            };
        };

        case "DODGE": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {                
                if (isNil "_subaction") then {
                    [
                        _unit, 
                        [
                            [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                            [0, "ACTION_PLAY_NOW", "KH_MeleeDodge", true], 
                            [0, "ACTION_PLAY", "KH_MeleeStop", true]
                        ], 
                        false,
                        false
                    ] call KH_fnc_setAnimation;
                }
                else {
                    [
                        _unit, 
                        [
                            [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                            [
                                0, 
                                "ACTION_PLAY_NOW",
                                switch _subaction do {
                                    case "FORWARD": {
                                        "KH_MeleeDodgeForward";
                                    };

                                    case "FORWARD_LEFTWARD": {
                                        "KH_MeleeDodgeForwardLeftward";
                                    };

                                    case "FORWARD_RIGHTWARD": {
                                        "KH_MeleeDodgeForwardRightward";
                                    };

                                    case "BACKWARD": {
                                        "KH_MeleeDodgeBackward";
                                    };

                                    case "BACKWARD_LEFTWARD": {
                                        "KH_MeleeDodgeBackwardLeftward";
                                    };

                                    case "BACKWARD_RIGHTWARD": {
                                        "KH_MeleeDodgeBackwardRightward";
                                    };

                                    case "LEFTWARD": {
                                        "KH_MeleeDodgeLeftward";
                                    };

                                    case "RIGHTWARD": {
                                        "KH_MeleeDodgeRightward";
                                    };

                                    default {
                                        "KH_MeleeDodgeBackward";
                                    };
                                },
                                true
                            ],
                            [0, "ACTION_PLAY", "KH_MeleeStop", true]
                        ], 
                        false,
                        false
                    ] call KH_fnc_setAnimation;                
                };
            }
            else {
                false;
            };
        };

        case "BLOCKED": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            [
                _unit, 
                [
                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                    [0, "ACTION_PLAY_NOW", "KH_MeleeBlocked", true], 
                    [0, "ACTION_PLAY", "KH_MeleeStop", true]
                ], 
                false,
                false
            ] call KH_fnc_setAnimation;
        };

        case "PARRIED": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];
            
            [
                _unit, 
                [
                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                    [0, "ACTION_PLAY_NOW", "KH_MeleeParried", true], 
                    [0, "ACTION_PLAY", "KH_MeleeStop", true]
                ], 
                false,
                false
            ] call KH_fnc_setAnimation;
        };

        case "CYCLE_ATTACK_MODE": {
            private _modes = getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModes");

            if (_modes isNotEqualTo []) then {
                private _currentModeIndex = _modes find (_unit getVariable ["KH_var_meleeMode", ""]);

                if !(isNil {_modes param [_currentModeIndex + 1];}) then {
                    _unit setVariable ["KH_var_meleeMode", _modes select (_currentModeIndex + 1), true];
                }
                else {
                    _unit setVariable ["KH_var_meleeMode", _modes select 0, true];
                };

                true;
            }
            else {
                false;
            };
        };

        case "HIT": {
            _subaction params ["_kickPower", "_tacklePower"];
            private _moves = getText ((configOf _unit) >> "moves");
            
            if (_modifier isEqualType objNull) then {
                _modifier = _unit getRelDir _modifier;
            };

            if KH_var_meleeAttacksInterruptAttacks then {
                if (
                    ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeAttackPhase")) isEqualTo 1) || 
                    ((getNumber (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _unit) >> "kh_meleeAttackPhase")) isEqualTo 1)
                   ) then {
                    [
                        _unit, 
                        [
                            [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                            [0, "ACTION_PLAY_NOW", "KH_MeleeBlocked", true]
                        ], 
                        false,
                        false
                    ] call KH_fnc_setAnimation;
                };
            };

            if KH_var_meleeAttacksInterruptKicks then {
                if ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeKickPhase")) isEqualTo 1) then {
                    if (_kickPower >= (getNumber (configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]) >> (_unit getVariable ["KH_var_currentMeleeKick", ""]) >> "power"))) then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_StaggerLightBackward", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    };
                };
            };

            if KH_var_meleeAttacksInterruptTackles then {
                if ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeTacklePhase")) isEqualTo 1) then {
                    if (_tacklePower >= (getNumber (configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]) >> (_unit getVariable ["KH_var_currentMeleeTackle", ""]) >> "power"))) then {
                        [
                            _unit, 
                            [
                                [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                                [0, "ACTION_PLAY_NOW", "KH_StaggerHeavyBackward", true]
                            ], 
                            false,
                            false
                        ] call KH_fnc_setAnimation;
                    };
                };
            };

            [
                _unit,
                [
                    "ACTION_PLAY_NOW",
                    switch true do {
                        case ((_modifier < 22.5) || (_modifier > 337.5)): {
                            "KH_MeleeHitFrontGesture";
                        };

                        case ((_modifier >= 22.5) && (_modifier < 67.5)): {
                            "KH_MeleeHitFrontRightGesture";
                        };

                        case ((_modifier >= 67.5) && (_modifier < 112.5)): {
                            "KH_MeleeHitRightGesture";
                        };

                        case ((_modifier >= 112.5) && (_modifier < 157.5)): {
                            "KH_MeleeHitBackRightGesture";
                        };

                        case ((_modifier >= 157.5) && (_modifier < 202.5)): {
                            "KH_MeleeHitBackGesture";
                        };

                        case ((_modifier >= 202.5) && (_modifier < 247.5)): {
                            "KH_MeleeHitBackLeftGesture";
                        };

                        case ((_modifier >= 247.5) && (_modifier < 292.5)): {
                            "KH_MeleeHitLeftGesture";
                        };

                        case ((_modifier >= 292.5) && (_modifier < 337.5)): {
                            "KH_MeleeHitFrontLeftGesture";
                        };

                        default {
                            "KH_MeleeHitFrontGesture";
                        };
                    },
                    true
                ],
                false,
                false
            ] call KH_fnc_setAnimation;
        };
    
        case "STAGGER": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            if (_modifier isEqualType objNull) then {
                _modifier = _unit getRelDir _modifier;
            };

            [
                _unit, 
                [
                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                    [
                        0, 
                        "ACTION_PLAY_NOW",
                        switch true do {
                            case ((_modifier < 22.5) || (_modifier > 337.5)): {
                                ["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select _subaction;
                            };

                            case ((_modifier >= 22.5) && (_modifier < 67.5)): {
                                ["KH_StaggerLightBackwardLeftward", "KH_StaggerHeavyBackwardLeftward"] select _subaction;
                            };

                            case ((_modifier >= 67.5) && (_modifier < 112.5)): {
                                ["KH_StaggerLightLeftward", "KH_StaggerHeavyLeftward"] select _subaction;
                            };

                            case ((_modifier >= 112.5) && (_modifier < 157.5)): {
                                ["KH_StaggerLightForwardLeftward", "KH_StaggerHeavyForwardLeftward"] select _subaction;
                            };

                            case ((_modifier >= 157.5) && (_modifier < 202.5)): {
                                ["KH_StaggerLightForward", "KH_StaggerHeavyForward"] select _subaction;
                            };

                            case ((_modifier >= 202.5) && (_modifier < 247.5)): {
                                ["KH_StaggerLightForwardRightward", "KH_StaggerHeavyForwardRightward"] select _subaction;
                            };

                            case ((_modifier >= 247.5) && (_modifier < 292.5)): {
                                ["KH_StaggerLightRightward", "KH_StaggerHeavyRightward"] select _subaction;
                            };

                            case ((_modifier >= 292.5) && (_modifier < 337.5)): {
                                ["KH_StaggerLightBackwardRightward", "KH_StaggerHeavyBackwardRightward"] select _subaction;
                            };

                            default {
                                ["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select _subaction;
                            };
                        },
                        true
                    ], 
                    [0, "ACTION_PLAY", "KH_MeleeStop", true]
                ], 
                false,
                false
            ] call KH_fnc_setAnimation;
        };

        case "STOP": {
            _unit setVariable ["KH_var_attackGestureIndex", -1];

            [
                _unit, 
                [
                    [0, "ACTION_PLAY_NOW", "KH_GestureNone", true],
                    [0, "ACTION_PLAY_NOW", "KH_MeleeStop", true]
                ], 
                false,
                false
            ] call KH_fnc_setAnimation;
        };

        default {
            false;
        };
    };
}
else {
    true;
};