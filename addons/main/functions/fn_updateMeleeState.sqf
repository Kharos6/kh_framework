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
    _subaction = _action param [1, "", [""]];
    _modifier = _action param [2, 0, [0, objNull]];
    _action = _action param [0, "", [""]];
};

if (_action isNotEqualTo "") then {
    switch _action do {
        case "ATTACK": {
            _action = _unit getVariable ["KH_var_meleeMode", ""];
            private _moves = getText ((configOf _unit) >> "moves");

            if (KH_var_meleeAttackAnimationTypePriority isEqualTo 0) then {
                if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> _action)) isNotEqualTo "") then {
                    _unit switchGesture ["GestureNone"];
                    _unit playActionNow _action;
                    _unit playAction "KH_MeleeStop";
                }
                else {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
                    _unit playActionNow ([_action, "Gesture"] joinString "");
                };
            }
            else {
                if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> ([_action, "Gesture"] joinString ""))) select 0) isNotEqualTo "") then {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
                    _unit playActionNow ([_action, "Gesture"] joinString "");
                }
                else {
                    _unit switchGesture ["GestureNone"];
                    _unit playActionNow _action;
                    _unit playAction "KH_MeleeStop";
                };
            };
        };

        case "BLOCK_IN": {
            private _moves = getText ((configOf _unit) >> "moves");

			if (KH_var_meleeBlockAnimationTypePriority isEqualTo 0) then {
				if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockIn")) isNotEqualTo "") then {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockIn";
				}
				else {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeBlockGesture";
				};
			}
			else {
				if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockGesture")) select 0) isNotEqualTo "") then {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeBlockGesture";
				}
				else {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockIn";
				};
			};
        };

        case "BLOCK_OUT": {
            if ((getArray (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> (animationState _unit) >> "kh_meleeBlockTiming")) isNotEqualTo []) then {
                _unit playActionNow "KH_MeleeBlockOut";
                _unit playAction "KH_MeleeStop";
            };

            _unit switchGesture ["GestureNone"];
        };

        case "BLOCK_SUCCESS": {
            private _moves = getText ((configOf _unit) >> "moves");

			if (KH_var_meleeBlockAnimationTypePriority isEqualTo 0) then {
				if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccess")) isNotEqualTo "") then {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockSuccess";
                    _unit playAction "KH_MeleeBlockIn";
				}
				else {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeBlockSuccessGesture";
                    _unit playAction "KH_MeleeBlockGesture";
				};
			}
			else {
				if (((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeBlockSuccessGesture")) select 0) isNotEqualTo "") then {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeBlockSuccessGesture";
                    _unit playAction "KH_MeleeBlockGesture";
				}
				else {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeBlockSuccess";
                    _unit playAction "KH_MeleeBlockIn";
				};
			};
        };
        
        case "PARRY": {
            private _moves = getText ((configOf _unit) >> "moves");

			if (KH_var_meleeParryAnimationTypePriority isEqualTo 0) then {
				if ((getText (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParry")) isNotEqualTo "") then {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeParry";
                    _unit playAction "KH_MeleeStop";
				}
				else {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeParryGesture";
				};
			}
			else {
				if ((getArray (configFile >> _moves >> "actions" >> (getText (configFile >> _moves >> "states" >> (animationState _unit) >> "actions")) >> "KH_MeleeParryGesture")) isNotEqualTo []) then {
                    if (_unit getVariable ["KH_var_meleeMoveActive", false]) exitWith {};
					_unit playActionNow "KH_MeleeParryGesture";
				}
				else {
                    _unit switchGesture ["GestureNone"];
					_unit playActionNow "KH_MeleeParry";
                    _unit playAction "KH_MeleeStop";
				};
			};
        };

        case "KICK": {
            _unit switchGesture ["GestureNone"];
			_unit playActionNow "KH_MeleeKick";
            _unit playAction "KH_MeleeStop";
        };

        case "TACKLE": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeTackle";
            _unit playAction "KH_MeleeStop";
        };

        case "DODGE": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeDodge";
            _unit playAction "KH_MeleeStop";
        };

        case "RECOIL": {
            _unit switchGesture ["GestureNone"];
            _unit playActionNow "KH_MeleeRecoil";
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
            if (_modifier isEqualType objNull) then {
                _modifier = _unit getRelDir _modifier;
            };

            if KH_var_meleeHitsInterruptAttacks then {
                if ((getArray (configFile >> _moves >> "states" >> (animationState _unit) >> "kh_meleeHitTiming")) isNotEqualTo []) then {
                    _unit playActionNow "KH_MeleeRecoil";
                };
            };

            switch true do {
                case ((_modifier < 45) && (_modifier > 315)): {
                    _unit playActionNow "KH_MeleeHitFrontGesture";
                };

                case ((_modifier < 135) && (_modifier > 45)): {
                    _unit playActionNow "KH_MeleeHitRightGesture";
                };

                case ((_modifier < 225) && (_modifier > 135)): {
                    _unit playActionNow "KH_MeleeHitBackGesture";
                };

                case ((_modifier < 315) && (_modifier > 225)): {
                    _unit playActionNow "KH_MeleeHitLeftGesture";
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
                case ((_modifier < 45) && (_modifier > 315)): {
                    _unit playActionNow (["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select (_subaction isEqualTo "HEAVY"));
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier < 135) && (_modifier > 45)): {
                    _unit playActionNow (["KH_StaggerLightLeftward", "KH_StaggerHeavyLeftward"] select (_subaction isEqualTo "HEAVY"));
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier < 225) && (_modifier > 135)): {
                    _unit playActionNow (["KH_StaggerLightForward", "KH_StaggerHeavyForward"] select (_subaction isEqualTo "HEAVY"));
                    _unit playAction "KH_MeleeStop";
                };

                case ((_modifier < 315) && (_modifier > 225)): {
                    _unit playActionNow (["KH_StaggerLightRightward", "KH_StaggerHeavyRightward"] select (_subaction isEqualTo "HEAVY"));
                    _unit playAction "KH_MeleeStop";
                };

                default {
                    _unit playActionNow (["KH_StaggerLightBackward", "KH_StaggerHeavyBackward"] select (_subaction isEqualTo "HEAVY"));
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