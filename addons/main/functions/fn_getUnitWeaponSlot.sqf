params [["_unit", objNull, [objNull]], ["_weapon", "", [""]]];

switch _weapon do {
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