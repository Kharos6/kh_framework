isNil {
	params [["_logic", objNull, [objNull]]];

	execute [
		[],
		{
            [
                KH_var_missionPersistencyIdentifier,
                [KH_var_missionPersistencyUnits, KH_var_missionPersistencyUnitsUndefined],
                KH_var_missionPersistencyGroups,
                [KH_var_missionPersistencyObjects, KH_var_missionPersistencyObjectsUndefined],
                [KH_var_missionPersistencyPlayers, KH_var_missionPersistencyPlayersDisconnected],
                KH_var_missionPersistencyMarkers,
                KH_var_missionPersistencyMission
            ] call KH_fnc_savePersistency;

            execute [[], {hint "MISSION SAVED";}, "GLOBAL", true, false];
		},
		"SERVER",
		true,
		false
	];

    deleteVehicle _logic;
};

nil;