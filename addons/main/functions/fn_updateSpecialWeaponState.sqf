params [["_unit", objNull, [objNull]]];
private _currentSelectedWeapon = currentWeapon _unit;

if (_currentSelectedWeapon isNotEqualTo "") then {
    private _currentWeaponSlot = [_unit] call KH_fnc_getCurrentWeaponSlot;
    private _surrogate = getText (configFile >> "CfgWeapons" >> _currentSelectedWeapon >> "kh_meleeSurrogate");
    private _currentSurrogateSlot = "";
    
    private _currentWeapon = if (_surrogate isNotEqualTo _currentWeaponSlot) then {
        _currentSurrogateSlot = _surrogate;
        
        switch _surrogate do {
            case "PRIMARY": {
                primaryWeapon _unit;
            };

            case "SECONDARY": {
                handgunWeapon _unit;
            };

            case "TERTIARY": {
                secondaryWeapon _unit;
            };

            default {
                _currentSelectedWeapon;
            };
        };
    }
    else {
        _currentSelectedWeapon;  
    };

    private _meleeAttachments = switch _currentSurrogateSlot do {
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

    if ((_unit getVariable ["KH_var_currentMeleeWeapon", ""]) isEqualTo _currentSelectedWeapon) exitWith {};
    _unit setVariable ["KH_var_currentMeleeWeapon", _currentSelectedWeapon];
    private _meleeState = (getNumber (configFile >> "CfgWeapons" >> _currentSelectedWeapon >> "kh_meleeWeapon")) isEqualTo 1;
    _unit setVariable ["KH_var_meleeWeaponConfig", _weaponConfig];
    _unit setVariable ["KH_var_meleeMode", (getArray (_weaponConfig >> "kh_meleeModes")) param [0, ""], true];
    _unit setVariable ["KH_var_inMeleeState", _meleeState];

    if ((getText (_weaponConfig >> "kh_specialActions")) isNotEqualTo "") then {
        _unit setVariable ["KH_var_meleeWeaponSlot", _currentWeaponSlot];
        
        [
            [
                _unit, 
                [
                    ["ACTION_SWITCH", "KH_GestureNone", true],
                    ["ACTION_SWITCH", [getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_specialActions"), "Transition"] joinString "", true], 
                    ["ACTION_PLAY", getText ((_unit getVariable ["KH_var_meleeWeaponConfig", configNull]) >> "kh_specialActions"), true]
                ], 
                false,
                false
            ],
            "KH_fnc_setAnimation",
            true,
            "-1",
            false
        ] call KH_fnc_execute;
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
    _unit setVariable ["KH_var_currentMeleeWeapon", ""];
};

nil;