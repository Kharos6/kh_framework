params [["_unit", objNull, [objNull]], ["_action", "", ["", []]]];

if (
    !(alive _unit) ||
    !(isNull (objectParent _unit)) || 
    !(isNull (attachedTo _unit)) ||
    ((lifeState _unit) isEqualTo "INCAPACITATED") || 
    ((lifeState _unit) isEqualTo "UNCONSCIOUS")
) exitWith {
    _unit setVariable ["KH_var_enteringMelee", false];
    _unit setVariable ["KH_var_meleeMode", "", true];
    _unit setVariable ["KH_var_rawMeleeStance", false];
    false;
};

if (((currentWeapon _unit) isNotEqualTo "") || (_unit getVariable ["KH_var_rawMeleeStance", false])) then {
    private _weaponConfig = if (_unit getVariable ["KH_var_rawMeleeStance", false]) then {
        private _config = configOf _unit;

        if ((_unit getVariable ["KH_var_meleeType", ""]) isNotEqualTo (getText (_config >> "kh_meleeType"))) then {
            _unit setVariable ["KH_var_meleeType", getText (_config >> "kh_meleeType"), true];
        };

        _config;
    }
    else {
        private _config = configFile >> "CfgWeapons" >> (currentWeapon _unit);

        if ((_unit getVariable ["KH_var_meleeType", ""]) isNotEqualTo (getText (_config >> "kh_meleeType"))) then {
            _unit setVariable ["KH_var_meleeType", getText (_config >> "kh_meleeType"), true];
        };

        _config;
    };

    _unit setVariable ["KH_var_meleeWeaponConfig", _weaponConfig];
    _unit setVariable ["KH_var_meleeMode", (getArray (_weaponConfig >> "kh_meleeModes")) param [0, ""], true];

    if ((getText (_weaponConfig >> "kh_meleeActions")) isNotEqualTo "") then {
        _unit setVariable ["KH_var_enteringMelee", true];
        _unit action ["SwitchWeapon", _unit, _unit, 299];
    };
}
else {
    if (_unit getVariable ["KH_var_enteringMelee", false]) then {
        _unit switchAction ([getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeActions"), "Transition"] joinString "");
        _unit playAction (getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeActions"));
        _unit setVariable ["KH_var_rawMeleeStance", false];
        _unit setVariable ["KH_var_enteringMelee", false, true];
    };
};

private "_subaction";
private "_modifier";

if (_action isEqualType []) then {
    _subaction = _action param [1, false, [true, []]];
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
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow _mode;
                        _unit playAction "KH_MeleeStop";
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
                                private _gestureAction = (getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> (_gestureMode param [_newIndex, []]))) select 0;
                                
                                if (_gestureAction isNotEqualTo "") then {
                                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                                        _unit setVariable ["KH_var_attackGestureIndex", _newIndex];
                                        _unit playActionNow _newMode;
                                    };
                                };
                            };
                        };
                    };
                }
                else {
                    private _gestureMode = (getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModesGestures")) param [(getArray ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_meleeModes")) find _mode, []];
                    
                    if (_gestureMode isEqualTo []) then {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow _mode;
                        _unit playAction "KH_MeleeStop";
                    }
                    else {
                        private _newIndex = (_unit getVariable ["KH_var_attackGestureIndex", -1]) + 1;
                        private _newMode = _gestureMode param [_newIndex, ""];

                        if (_newMode isEqualTo "") then {
                            _newIndex = 0;
                            _newMode = _gestureMode param [_newIndex, ""];
                        };

                        if (_newMode isEqualTo "") then {
                            _unit switchGesture ["GestureNone"];
                            _unit playActionNow _mode;
                            _unit playAction "KH_MeleeStop";
                        }
                        else {
                            private _gestureAction = (getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> (_gestureMode param [_newIndex, []]))) select 0;

                            if (_gestureAction isEqualTo "") then {
                                _unit switchGesture ["GestureNone"];
                                _unit playActionNow _mode;
                                _unit playAction "KH_MeleeStop";
                            }
                            else {
                                if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                                    _unit setVariable ["KH_var_attackGestureIndex", _newIndex];
                                    _unit playActionNow _newMode;
                                };
                            };
                        };
                    };
                };
            };
        };

        case "BLOCK_IN": {
            private _moves = getText ((configOf _unit) >> "moves");

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                private _priority = [KH_var_meleeAiBlockAnimationTypePriority, KH_var_meleePlayerBlockAnimationTypePriority] select (isPlayer _unit);

                if (_priority isEqualTo 0) then {
                    if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockIn")) isNotEqualTo "") then {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_MeleeBlockIn";
                    }
                    else {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            _unit playActionNow "KH_MeleeBlockGesture";
                        };
                    };
                }
                else {
                    if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockGesture")) select 0) isNotEqualTo "") then {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            _unit playActionNow "KH_MeleeBlockGesture";
                        };
                    }
                    else {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_MeleeBlockIn";
                    };
                };
            };
        };

        case "BLOCK_OUT": {
            private _moves = getText ((configOf _unit) >> "moves");
            
            if ((getArray (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeBlockTiming")) isNotEqualTo []) then {
                _unit switchGesture ["GestureNone"];
                _unit playActionNow "KH_MeleeBlockOut";
                _unit playAction "KH_MeleeStop";
            };

            if ((getArray (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _unit) >> "kh_meleeBlockTiming")) isNotEqualTo []) then {   
                _unit switchGesture ["GestureNone"];
            };
        };

        case "BLOCK_SUCCESS": {
            private _moves = getText ((configOf _unit) >> "moves");
            private _priority = [KH_var_meleeAiBlockAnimationTypePriority, KH_var_meleePlayerBlockAnimationTypePriority] select (isPlayer _unit);

			if (_priority isEqualTo 0) then {
				if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccess")) isNotEqualTo "") then {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockSuccess";
                    _unit playAction "KH_MeleeBlockIn";
				}
				else {
                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                        _unit playActionNow "KH_MeleeBlockSuccessGesture";
                        _unit playAction "KH_MeleeBlockGesture";
                    };
				};
			}
			else {
				if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccessGesture")) select 0) isNotEqualTo "") then {
                    if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                        _unit playActionNow "KH_MeleeBlockSuccessGesture";
                        _unit playAction "KH_MeleeBlockGesture";
                    };
				}
				else {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockSuccess";
                    _unit playAction "KH_MeleeBlockIn";
				};
			};
        };

        case "BLOCK_FAILURE": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeBlockFailure";
            _unit playAction "KH_MeleeStop";
        };
        
        case "PARRY": {
            private _moves = getText ((configOf _unit) >> "moves");

            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                private _priority = [KH_var_meleeAiParryAnimationTypePriority, KH_var_meleePlayerParryAnimationTypePriority] select (isPlayer _unit);

                if (_priority isEqualTo 0) then {
                    if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParry")) isNotEqualTo "") then {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_MeleeParry";
                        _unit playAction "KH_MeleeStop";
                    }
                    else {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            _unit playActionNow "KH_MeleeParryGesture";
                        };
                    };
                }
                else {
                    if ((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParryGesture")) isNotEqualTo []) then {
                        if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
                            _unit playActionNow "KH_MeleeParryGesture";
                        };
                    }
                    else {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_MeleeParry";
                        _unit playAction "KH_MeleeStop";
                    };
                };
            };
        };

        case "KICK": {
            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                _unit switchGesture ["GestureNone"];
                _unit playActionNow "KH_MeleeKick";
                _unit playAction "KH_MeleeStop";
            };
        };

        case "TACKLE": {
            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                _unit switchGesture ["GestureNone"];
                _unit playActionNow "KH_MeleeTackle";
                _unit playAction "KH_MeleeStop";
            };
        };

        case "DODGE": {
            if !(_unit getVariable ["KH_var_meleeGestureActive", false]) then {
                _unit switchGesture ["GestureNone"];
                _unit playActionNow "KH_MeleeDodge";
                _unit playAction "KH_MeleeStop";
            };
        };

        case "BLOCKED": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeBlocked";
            _unit playAction "KH_MeleeStop";
        };

        case "PARRIED": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeParried";
            _unit playAction "KH_MeleeStop";
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
            };
        };

        case "HIT": {
            _subaction params ["_kickPower", "_tacklePower"];
            private _moves = getText ((configOf _unit) >> "moves");
            
            if (_modifier isEqualType objNull) then {
                _modifier = _unit getRelDir _modifier;
            };

            if KH_var_meleeAttacksInterruptAttacks then {
                if ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeAttackPhase")) isEqualTo 1) then {
                    _unit switchGesture ["GestureNone"];
                    _unit playActionNow "KH_MeleeBlocked";
                };
            };

            if KH_var_meleeAttacksInterruptKicks then {
                if ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeKickPhase")) isEqualTo 1) then {
                    if (_kickPower >= (getNumber (configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]) >> (_unit getVariable ["KH_var_currentMeleeKick", ""]) >> "power"))) then {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_StaggerLightBackward";
                    };
                };
            };

            if KH_var_meleeAttacksInterruptTackles then {
                if ((getNumber (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeTacklePhase")) isEqualTo 1) then {
                    if (_tacklePower >= (getNumber (configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]) >> (_unit getVariable ["KH_var_currentMeleeTackle", ""]) >> "power"))) then {
                        _unit switchGesture ["GestureNone"];
                        _unit playActionNow "KH_StaggerHeavyBackward";
                    };
                };
            };

            switch true do {
                case ((_modifier < 22.5) || (_modifier > 337.5)): {
                    _unit playActionNow "KH_MeleeHitFrontGesture";
                };

                case ((_modifier >= 22.5) && (_modifier < 67.5)): {
                    _unit playActionNow "KH_MeleeHitFrontRightGesture";
                };

                case ((_modifier >= 67.5) && (_modifier < 112.5)): {
                    _unit playActionNow "KH_MeleeHitRightGesture";
                };

                case ((_modifier >= 112.5) && (_modifier < 157.5)): {
                    _unit playActionNow "KH_MeleeHitBackRightGesture";
                };

                case ((_modifier >= 157.5) && (_modifier < 202.5)): {
                    _unit playActionNow "KH_MeleeHitBackGesture";
                };

                case ((_modifier >= 202.5) && (_modifier < 247.5)): {
                    _unit playActionNow "KH_MeleeHitBackLeftGesture";
                };

                case ((_modifier >= 247.5) && (_modifier < 292.5)): {
                    _unit playActionNow "KH_MeleeHitLeftGesture";
                };

                case ((_modifier >= 292.5) && (_modifier < 337.5)): {
                    _unit playActionNow "KH_MeleeHitFrontLeftGesture";
                };

                default {
                    _unit playActionNow "KH_MeleeHitFrontGesture";
                };
            };
        };
    
        case "STAGGER": {
            _unit switchGesture ["GestureNone"];

            if (_modifier isEqualType objNull) then {
                _modifier = _unit getRelDir _modifier;
            };

            switch true do {
                case ((_modifier < 22.5) || (_modifier > 337.5)): {
                    _unit playActionNow (["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 22.5) && (_modifier < 67.5)): {
                    _unit playActionNow (["KH_StaggerLightBackwardLeftward", "KH_StaggerHeavyBackwardLeftward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 67.5) && (_modifier < 112.5)): {
                    _unit playActionNow (["KH_StaggerLightLeftward", "KH_StaggerHeavyLeftward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 112.5) && (_modifier < 157.5)): {
                    _unit playActionNow (["KH_StaggerLightForwardLeftward", "KH_StaggerHeavyForwardLeftward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 157.5) && (_modifier < 202.5)): {
                    _unit playActionNow (["KH_StaggerLightForward", "KH_StaggerHeavyForward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 202.5) && (_modifier < 247.5)): {
                    _unit playActionNow (["KH_StaggerLightForwardRightward", "KH_StaggerHeavyForwardRightward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 247.5) && (_modifier < 292.5)): {
                    _unit playActionNow (["KH_StaggerLightRightward", "KH_StaggerHeavyRightward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier >= 292.5) && (_modifier < 337.5)): {
                    _unit playActionNow (["KH_StaggerLightBackwardRightward", "KH_StaggerHeavyBackwardRightward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };

                default {
                    _unit playActionNow (["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select _subaction);
                    _unit playAction "KH_MeleeStop";
                };
            };
        };

        case "STOP": {
            _unit playActionNow "KH_MeleeStop";
            _unit switchGesture ["GestureNone"];
        };
    };
};

true;