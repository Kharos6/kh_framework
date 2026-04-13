params [["_unit", objNull, [objNull]], ["_type", "PRIMARY", [""]]];

switch _type do {
    case "PRIMARY": {
        _unit modelToWorldVisual (_unit selectionPosition (getText ((configOf _unit) >> "weaponBone")));
    };

    case "SECONDARY": {
        _unit modelToWorldVisual (_unit selectionPosition (getText ((configOf _unit) >> "handGunBone")));
    };

    case "TERTIARY": {
        _unit modelToWorldVisual (_unit selectionPosition (getText ((configOf _unit) >> "launcherBone")));
    };

    default {
        ASLToAGL (eyePos _unit);
    };
};