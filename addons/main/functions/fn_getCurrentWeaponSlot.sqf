params [["_unit", objNull, [objNull]]];
private _currentWeapon = currentWeapon _unit;

if (_currentWeapon isEqualTo "") exitWith {
    "NONE";
};

switch _currentWeapon do {
    case (primaryWeapon _unit): {
        "PRIMARY";
    };

    case (handgunWeapon _unit): {
        "SECONDARY";
    };

    case (secondaryWeapon _unit): {
        "TERTIARY";
    };

    default {
        "NONE";
    };
};