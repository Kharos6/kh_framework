params ["_missionState", ["_objects", true], ["_players", [true, "INITIAL"]], ["_units", true], ["_agents", true], ["_world", true]];

private _savedMissionState = [];

if (_missionState isEqualType []) then {
	_savedMissionState = _missionState;
}
else {
	_savedMissionState = missionProfileNamespace getVariable [format ["KH_var_savedMissionState_%1", _missionState], []];
};

private _savedObjects = _savedMissionState select 0;
private _cargoObjects = createHashMapFromArray (_savedMissionState select 1);
private _playerLoadouts = createHashMapFromArray (_savedMissionState select 2);
private _savedUnits = createHashMapFromArray (_savedMissionState select 3);
private _savedAgents = _savedMissionState select 4;
private _savedWorldInfo = _savedMissionState select 5;
private _enabledObjects = [];
private _dynamicObjects = [];
private _simpleObjects = [];
private _vehicleCrew = [];
private _oldTerrainObjects = [];

if _objects then {
	{
		private _object = createVehicle [_x select 0, _x select 2, [], 0, "NONE"];
		_object enableSimulationGlobal false;
		_object setVectorDirAndUp [_x select 3, _x select 4];

		[
			[_object, _x select 1],
			{
				params ["_object", "_variableName"];
				_object setVehicleVarName _variableName;
			},
			["JIP", "GLOBAL", _object, false, false],
			"THIS_FRAME"
		] call KH_fnc_execute;
		
		_object setDamage (_x select 5);
		private _i = 0;
		private _hitpointCount = (count ((_x select 6) select 0)) - 1;
		
		for "_i" from 0 to _hitpointCount do {			
			_object setHitPointDamage [(((_x select 6) select 0) select _i), (((_x select 6) select 1) select _i)];
		};
		
		_object setFuel (_x select 7);
		_object setAmmoCargo (_x select 8);
		_object setFuelCargo (_x select 9);
		_object setRepairCargo (_x select 10);
		
		if (_x select 11) then {
			_enabledObjects pushBack _object;
		};
		
		if (_x select 12) then {
			_dynamicObjects pushBack _object;
		};
		
		if (_x select 13) then {
			_simpleObjects pushBack _object;
		};
		
		if ((_x select 14) isNotEqualTo "[]") then {
			_vehicleCrew pushBack [_object, _x select 14];
		};
		
		_oldTerrainObjects pushBack (nearestTerrainObjects [_object, [], 0.01, false, true]);
	} forEach _savedObjects;

	{
		private _oldObjects = _x;

		{
			deleteVehicle _x;
		} forEach _oldObjects;
	} forEach _oldTerrainObjects;

	[_simpleObjects, false, {}] call KH_fnc_convertToSimpleObject;

	{
		_x enableSimulationGlobal true;
	} forEach _enabledObjects;

	{
		[
			[_x],
			{
				params ["_object"];
				_object enableDynamicSimulation true;
			},
			["JIP", "GLOBAL", _x, false, false],
			"THIS_FRAME"
		] call KH_fnc_execute;
	} forEach _dynamicObjects;

	[_cargoObjects] call KH_fnc_loadCargoInventories;
};

if (_players select 0) then {
	[_playerLoadouts, _players select 1] call KH_fnc_loadPlayerLoadouts;
};

if _units then {
	{
		private _group = createGroup [_y select 2, true];
		
		{
			private _unit = _group createUnit [_y select 0, [0, 0, 0], [], 0, "CAN_COLLIDE"];
			[_unit, _y, false, true, true, true, true, true, false, true, true, true, true] call KH_fnc_setUnitAttributes;
		} forEach _y;
	} forEach _savedUnits;

	if _objects then {
		{
			_x params ["_vehicle", "_crew"];
			private _crewCompiled = parseSimpleArray _crew;

			{
				_x moveInAny _vehicle;
			} forEach _crewCompiled;
		} forEach _vehicleCrew;
	};
};

if _agents then {
	{
		private _agent = createAgent [_x select 0, [0, 0, 0], [], 0, "CAN_COLLIDE"];
		[_agent, _x, false, true, true, true, true, true, false, true, true, true, true] call KH_fnc_setUnitAttributes;
	} forEach _savedAgents;
};

if _world then {
	setDate (_savedWorldInfo select 0);
	0 setOvercast (_savedWorldInfo select 1);
	0 setFog (_savedWorldInfo select 2);
	0 setRain (_savedWorldInfo select 3);
	0 setLightnings (_savedWorldInfo select 4);
	0 setWaves (_savedWorldInfo select 5);
	setWind (_savedWorldInfo select 6);
};

true;