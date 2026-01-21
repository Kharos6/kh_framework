params [["_identifier", "", [""]]];

if (_identifier isEqualTo "") exitWith {
    [];
};

KH_var_khDataNamespace writeKhData [["missionPersistency_", _identifier] joinString "", call KH_fnc_getMissionAttributes];