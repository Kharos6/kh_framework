params [["_unit", objNull, [objNull]]];

if ((currentWeapon _unit) isEqualTo "") then {
    false;
}
else {
    (getNumber (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> (animationState _unit) >> "disableWeaponsLong")) isEqualTo 0;
};