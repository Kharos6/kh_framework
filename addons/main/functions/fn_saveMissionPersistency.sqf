params [["_identifier", "", [""]]];

if (_identifier isEqualTo "") exitWith {
    [];
};

([_identifier, "persistency"] joinString "_") writeKhData ["missionPersistency", call KH_fnc_getMissionAttributes];