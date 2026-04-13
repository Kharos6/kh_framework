params [["_unit", objNull, [objNull]]];

switch (currentWeapon _unit) do {
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