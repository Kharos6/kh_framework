params [["_identifier", "", [""]]];
if (_identifier isEqualTo "") exitWith {};
("khNamespace" readKhData [["missionPersistency_", _identifier] joinString "", []]) call KH_fnc_setMissionAttributes;
nil;