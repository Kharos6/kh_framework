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

private _weaponConfig = configFile >> "CfgWeapons" >> (currentWeapon _unit);

if ((getText (_weaponConfig >> "kh_meleeMoves")) isNotEqualTo "") then {
    _unit playActionNow "Civil";
    _unit playAction (getText (_weaponConfig >> "kh_meleeMoves"));
};

true;