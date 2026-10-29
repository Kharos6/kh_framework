params [["_unit", objNull, [objNull]], ["_allowRagdoll", false, [true]], ["_allowIncapacitated", false, [true]], ["_allowVehicle", true, [true]], ["_allowAirbourne", true, [true]], ["_allowUnderwater", true, [true]]];
private _alive = alive _unit;

if !_alive exitWith {
    false;
};

if (!_allowRagdoll && (_alive isNotEqualTo (isAwake _unit))) exitWith {
    false;
};

if (!_allowIncapacitated && ((lifeState _unit) in ["UNCONSCIOUS", "INCAPACITATED"])) exitWith {
    false;
};

if (!_allowVehicle && !(isNull (objectParent _unit))) exitWith {
    false;
};

if (!_allowAirbourne && !(isTouchingGround _unit)) exitWith {
    false;
};

if (!_allowUnderwater && (((eyePos _unit) select 2) < 0)) exitWith {
    false;
};

true;