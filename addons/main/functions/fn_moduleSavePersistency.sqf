isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
		[
            KH_var_missionPersistencyIdentifier,
            [KH_var_missionPersistencyUnits, KH_var_missionPersistencyUnitsUndefined],
            KH_var_missionPersistencyGroups,
            [KH_var_missionPersistencyObjects, KH_var_missionPersistencyObjectsUndefined],
            KH_var_missionPersistencyPlayers,
            KH_var_missionPersistencyMission
        ] call KH_fnc_savePersistency;
	};
};

nil;