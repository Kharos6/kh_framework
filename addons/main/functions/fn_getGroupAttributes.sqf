params [["_group", grpNull, [grpNull]]];

[
	KH_var_serverMissionSessionId,
	[_group, true] call KH_fnc_getEntityVariableName,
	_group getVariable ["KH_var_generatedVariableName", false],
	side _group,
	[
		[leader _group, true] call KH_fnc_getEntityVariableName,
		groupId _group,
		isGroupDeletedWhenEmpty _group,
		formation _group,
		combatBehaviour _group,
		combatMode _group,
		speedMode _group,
		(waypoints _group) apply {
			[
				[waypointAttachedVehicle _x, true] call KH_fnc_getEntityVariableName, 
				waypointBehaviour _x, waypointCombatMode _x, waypointCompletionRadius _x, waypointDescription _x, waypointForceBehaviour _x, waypointFormation _x, 
				waypointHousePosition _x, waypointLoiterAltitude _x, waypointLoiterRadius _x, waypointLoiterType _x, waypointName _x, waypointPosition _x, 
				waypointScript _x, waypointSpeed _x, waypointStatements _x, waypointTimeout _x, waypointType _x, waypointVisible _x
			];
		},	
		_group getVariable ["KH_var_persistentVariables", []]
	]
];