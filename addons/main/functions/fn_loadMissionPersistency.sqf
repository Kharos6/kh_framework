params [["_identifier", "", [""]]];
if (_identifier isEqualTo "") exitWith {};
private _savedPersistency = ([_identifier, "persistency"] joinString "_") readKhData ["missionPersistency", []];

if (_savedPersistency isNotEqualTo []) then {
    [_savedPersistency, []] call KH_fnc_setMissionAttributes;
};

nil;