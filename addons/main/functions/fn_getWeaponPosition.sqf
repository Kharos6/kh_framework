params [["_unit", objNull, [objNull]], ["_type", "PRIMARY", [""]]];

private _currentWeaponSelection = switch _type do {
    case "PRIMARY": {
        ["weapon", "RightHand", "weapon_ranged_r", "barrel_primary"];
    };

    case "SECONDARY": {
        ["RightHand", "weapon_ranged_l", "barrel_secondary"];
    };

    case "TERTIARY": {
        ["launcher", "RightHand", "weapon_melee_r"];
    };

    default {
        [];
    };
};

private _weaponPosition = ASLToAGL (eyePos _unit);

{
    private _currentPosition = _unit modelToWorldVisual (_unit selectionPosition _x);

    if (_currentPosition isNotEqualTo [0, 0, 0]) then {
        _weaponPosition = _currentPosition;
        break;
    };
} forEach _currentWeaponSelection;

_weaponPosition;