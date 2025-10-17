params [["_identifier", "", [""]]];

if (_identifier isEqualTo "") exitWith {
    [];
};

"khNamespace" writeKhData [["missionPersistency_", _identifier] joinString "", call KH_fnc_getMissionAttributes];