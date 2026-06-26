params [["_unit", objNull, [objNull]]];

if (
    ((currentWeapon _unit) isEqualTo "") || 
    (((weaponState _unit) select 6) > 0) || 
    (isSwitchingWeapon _unit) || 
    !(alive _unit) || 
    ((lifeState _unit) isEqualTo "INCAPACITATED") || 
    ((lifeState _unit) isEqualTo "UNCONSCIOUS")
   ) then {
    false;
}
else {
    (getNumber ((_unit getVariable ["KH_var_movesStatesConfig", configNull]) >> (animationState _unit) >> "disableWeaponsLong")) isEqualTo 0;
};