params [["_identifier", "", [""]]];
if (_identifier isEqualTo "") exitWith {};
private _savedPersistency = "khNamespace" readKhData [["missionPersistency_", _identifier] joinString "", []];

if (_savedPersistency isNotEqualTo []) then {
    [_savedPersistency, []] call KH_fnc_setMissionAttributes;
};

nil;