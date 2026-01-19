params [["_identifier", "", [""]]];

if (_identifier isEqualTo "") exitWith {
    [];
};

"kh_namespace" writeKhData [["missionPersistency_", _identifier] joinString "", call KH_fnc_getMissionAttributes];