isNil {
	KH_var_missionLoaded = true;

	if isServer then {
		if (KH_var_entityArrayBuilderArrays isNotEqualTo []) then {
			{				
				missionNamespace setVariable [_x, missionNamespace getVariable [_x, []], true];
			} forEach KH_var_entityArrayBuilderArrays;
		};

		if (KH_var_groupArrayBuilderArrays isNotEqualTo []) then {
			{				
				missionNamespace setVariable [_x, missionNamespace getVariable [_x, []], true];
			} forEach KH_var_groupArrayBuilderArrays;
		};

		if (KH_var_initialSideRelations isNotEqualTo []) then {
			{
				_x call KH_fnc_setSideRelations;
			} forEach KH_var_initialSideRelations;
		};

		call KH_fnc_serverMissionLoadInit;
		[[], {systemChat "KH FRAMEWORK - MISSION LOADED"; diag_log "KH FRAMEWORK - MISSION LOADED";}, ["SERVER", KH_var_currentAdmin] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;

		[
			[],
			{
				KH_var_missionStarted = true;
				publicVariable "KH_var_missionStarted";
				call KH_fnc_serverMissionStartInit;
				[[], "KH_fnc_playerMissionStartInit", "PLAYERS", true, false] call KH_fnc_execute;
				[[], "KH_fnc_headlessMissionStartInit", "HEADLESS", true, false] call KH_fnc_execute;
				[[], {systemChat "KH FRAMEWORK - MISSION STARTED"; diag_log "KH FRAMEWORK - MISSION STARTED";}, ["SERVER", KH_var_currentAdmin] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;

				[
					[],
					{
						KH_var_initialPlayerUnits = +KH_var_allPlayerUnits;
						publicVariable "KH_var_initialPlayerUnits";
						KH_var_initialPlayerMachines = +KH_var_allPlayerMachines;
						publicVariable "KH_var_initialPlayerMachines";
						KH_var_playersLoaded = true;
						publicVariable "KH_var_playersLoaded";
						call KH_fnc_serverPlayersLoadedInit;
						[[], "KH_fnc_playerPlayersLoadedInit", "PLAYERS", true, false] call KH_fnc_execute;
						[[], "KH_fnc_headlessPlayersLoadedInit", "HEADLESS", true, false] call KH_fnc_execute;
						["KH_eve_playersLoaded", []] call CBA_fnc_globalEvent;
						[[], {systemChat "KH FRAMEWORK - PLAYERS LOADED"; diag_log "KH FRAMEWORK - PLAYERS LOADED";}, ["SERVER", KH_var_currentAdmin] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;
					},
					true,
					{
						private _initialPlayerCount = 0;

						{
							if (!(_x getUserInfo 7) && ((_x getUserInfo 6) > 8)) then {
								_initialPlayerCount = _initialPlayerCount + 1;
							};
						} forEach allUsers;
						
						((((count KH_var_allPlayerUnits) >= _initialPlayerCount)) || (CBA_missionTime > 60) || !isMultiplayer);
					},
					false
				] call KH_fnc_execute;
			},
			true,
			{time > 0;},
			false
		] call KH_fnc_execute;
	}
	else {
		if KH_var_missionStarted then {
			KH_var_jip = true;
		};
	};
	
	if hasInterface then {
		"KH_var_khDisplayLayer" cutRsc ["KH_ResourceKHDisplay", "PLAIN", -1, true, true];
		call KH_fnc_playerMissionLoadInit;
		["KH_eve_playerMissionPreloaded", [clientOwner]] call CBA_fnc_serverEvent;
	};

	if (!isServer && !hasInterface) then {
		call KH_fnc_headlessMissionLoadInit;
		["KH_eve_headlessMissionPreloaded", [clientOwner]] call CBA_fnc_serverEvent;
	};

	{
		_x params ["_arguments", ["_function", {}, [{}]], ["_delay", 0, [0]]];

		if (_delay isEqualTo 0) then {
			if (isNil "_arguments") then {
				call _function;
			}
			else {
				_arguments call _function;
			};
		}
		else {
			_arguments = [_delay, _arguments, _function];

			[
				_arguments,
				{
					(_this select [1]) params ["_arguments", "_function"];

					if (isNil "_arguments") then {
						call _function;
					}
					else {
						_arguments call _function;
					};
				}, 
				true, 
				{
					params ["_delay"];
					time > _delay;
				}, 
				false
			] call KH_fnc_execute;
		};
	} forEach KH_var_postInitExecutions;

	{
		luaExecute _x;
	} forEach KH_var_postInitLuaExecutions;

	if isDedicated then {
		{
			luaExecute _x;
		} forEach KH_var_loadInitLuaExecutions;
	};
};

nil;