params [["_unit", objNull, [objNull]]];

if (
    !(alive _unit) ||
    !(isNull (objectParent _unit)) || 
    !(isNull (attachedTo _unit)) ||
    ((lifeState _unit) isEqualTo "INCAPACITATED") || 
    ((lifeState _unit) isEqualTo "UNCONSCIOUS") ||
    (_unit getVariable ["KH_var_incapacitated", false])
) exitWith {
    false;
};

private _weapon = currentWeapon _unit;

if ((getText (configFile >> "CfgWeapons" >> _weapon >> "kh_meleeHitType")) isNotEqualTo "") then {
    _unit playActionNow (["KH_MeleeIn", getText (configFile >> "CfgWeapons" >> _weapon >> "kh_meleeHitType")] joinString "");
}
else {
    _unit playActionNow "KH_MeleeOut";
};

private _weaponConfig = configFile >> "CfgWeapons" >> _weapon;
private _unitConfig = configOf _unit;

_unit setVariable [
    "KH_var_meleeAttributes",
    [
        if (isNumber (_weaponConfig >> "kh_meleeCostMultiplier")) then {
            getNumber (_weaponConfig >> "kh_meleeCostMutiplier");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeCostMultiplier");
        },
        if (isText (_weaponConfig >> "kh_meleeHitType")) then {
            getText (_weaponConfig >> "kh_meleeHitType");
        }
        else {
            getText (_unitConfig >> "kh_meleeHitType");
        },
        if (isNumber (_weaponConfig >> "kh_meleeHitRadius")) then {
            getNumber (_weaponConfig >> "kh_meleeHitRadius");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeHitRadius");
        },
        if (isNumber (_weaponConfig >> "kh_meleeHitBlockPower")) then {
            getNumber (_weaponConfig >> "kh_meleeHitBlockPower");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeHitBlockPower");
        },
        if (isNumber (_weaponConfig >> "kh_meleeHitParryPower")) then {
            getNumber (_weaponConfig >> "kh_meleeHitParryPower");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeHitParryPower");
        },
        if (isNumber (_weaponConfig >> "kh_meleeBlockAngle")) then {
            getNumber (_weaponConfig >> "kh_meleeBlockAngle");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeBlockAngle");
        },
        if (isNumber (_weaponConfig >> "kh_meleeBlockPower")) then {
            getNumber (_weaponConfig >> "kh_meleeBlockPower");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeBlockPower");
        },
        if (isNumber (_weaponConfig >> "kh_meleeParryAngle")) then {
            getNumber (_weaponConfig >> "kh_meleeParryAngle");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeParryAngle");
        },
        if (isNumber (_weaponConfig >> "kh_meleeParryPower")) then {
            getNumber (_weaponConfig >> "kh_meleeParryPower");
        }
        else {
            getNumber (_unitConfig >> "kh_meleeParryPower");
        },
        getText (_unitConfig >> "kh_meleeKickType"),
        getNumber (_unitConfig >> "kh_meleeKickPower"),
        getText (_unitConfig >> "kh_meleeTackleType"),
        getNumber (_unitConfig >> "kh_meleeTacklePower"),
        if (isText (_weaponConfig >> "kh_meleeDamageFunction")) then {
            getText (_weaponConfig >> "kh_meleeDamageFunction");
        }
        else {
            getText (_unitConfig >> "kh_meleeDamageFunction");
        }
    ],
    true
];

true;