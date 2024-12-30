isNil {
	KH_var_missionLoaded = true;

	if isServer then {
		if (KH_var_entityArrayBuilderArrays isNotEqualTo []) then {
			{				
				missionNamespace setVariable [_x, missionNamespace getVariable [_x, []], true];
			} forEach KH_var_entityArrayBuilderArrays;

			addMissionEventHandler [
				"EntityRespawned", 
				{
					params ["_newEntity", "_oldEntity"];
					{
						private _entityArray = missionNamespace getVariable [_x, []];

						if (_oldEntity in _entityArray) then {
							_entityArray deleteAt (_entityArray find _oldEntity);
							_entityArray pushBackUnique _newEntity;
							missionNamespace setVariable [_x, _entityArray, true];
						};
					} forEach KH_var_entityArrayBuilderArrays;
				}
			];

			[
				"KH_eve_playerLoaded", 
				{
					params ["_player"];
					private _arrayBuilderArray = _player getVariable ["KH_var_assignedEntityArrayBuilderArrays", []];

					if (_arrayBuilderArray isNotEqualTo []) then {
						{
							private _entityArray = missionNamespace getVariable [_x, []];

							if !(_player in _entityArray) then {
								_entityArray pushBackUnique _player;
								missionNamespace setVariable [_x, _entityArray, true];
							};
						} forEach _arrayBuilderArray;
					};
				}
			] call CBA_fnc_addEventHandler;
		};

		if (KH_var_groupArrayBuilderArrays isNotEqualTo []) then {
			{				
				missionNamespace setVariable [_x, missionNamespace getVariable [_x, []], true];
			} forEach KH_var_groupArrayBuilderArrays;
		};

		if (KH_var_dynamicSimulationEntities isNotEqualTo []) then {
			KH_var_dynamicSimulationSets = createHashMap;

			{
				_x params ["_entity", "_distance"];
				private _value = KH_var_dynamicSimulationSets get _distance;

				if !(isNil "_value") then {
					_value pushBack _entity;
					KH_var_dynamicSimulationSets set [_distance, _value];
				}
				else {
					KH_var_dynamicSimulationSets insert [[_distance, [_entity]]];
				};
			} forEach KH_var_dynamicSimulationEntities;

			{
				[_y, _x] call KH_fnc_dynamicSimulation;
			} forEach KH_var_dynamicSimulationSets;
		};

		if (KH_var_headlessClientTransfers isNotEqualTo []) then {
			{
				private _headlessClient = _x;
				private _assignedEntities = [];
				private _assignedEntitiesRecreate = [];

				{
					if ((vehicleVarName _headlessClient) in _x) then {
						private _recreate = _x select 2;

						if !_recreate then {
							_assignedEntities pushBack (_x select 0);
						}
						else {
							_assignedEntitiesRecreate pushBack (_x select 0);
						};
					};
				} forEach KH_var_headlessClientTransfers;
				
				if ((owner _headlessClient) == 2) then {
					[
						{
							((owner _headlessClient) != 2);
						},
						{
							params ["_assignedEntities", "_assignedEntitiesRecreate", "_headlessClient"];
							[_assignedEntities, owner _headlessClient, false, {}] call KH_fnc_setOwnership;
							[_assignedEntitiesRecreate, owner _headlessClient, true, {}] call KH_fnc_setOwnership;
						},
						[_assignedEntities, _assignedEntitiesRecreate, _headlessClient]
					] call CBA_fnc_waitUntilAndExecute;
				}
				else {
					params ["_assignedEntities", "_assignedEntitiesRecreate", "_headlessClient"];
					[_assignedEntities, owner _headlessClient, false, {}] call KH_fnc_setOwnership;
					[_assignedEntitiesRecreate, owner _headlessClient, true, {}] call KH_fnc_setOwnership;	
				};

				{
					[
						{
							params ["_unit", "_ownerId"];
							((owner _unit) == _ownerId);
						},
						{
							params ["_unit", "_ownerId"];
							[[_unit], _unit getVariable ["KH_var_headlessClientTransferInit", {}], _ownerId, "THIS_FRAME"] call KH_fnc_execute;
						}, 
						[_x, owner _headlessClient], 
						30
					] call CBA_fnc_waitUntilAndExecute;
				} forEach (_assignedEntities + _assignedEntitiesRecreate);
			} forEach ([["HEADLESS"], true] call KH_fnc_getClients);
		};

		if (KH_var_initialSideRelations isNotEqualTo []) then {
			private _relationFunction = {
				params ["_side1", "_side2", "_value"];
				if _value then {
					_side1 setFriend [_side2, 1];
				}
				else {
					_side1 setFriend [_side2, 0];
				};
			};

			{
				[_x select 0, _x select 1, _x select 2] call _relationFunction;
			} forEach KH_var_initialSideRelations;
		};
	
		[] call KH_fnc_serverMissionLoadInit;
		
		[
			[], 
			{
				systemChat "KH FRAMEWORK - MISSION INITIALIZED";
			}, 
			"GLOBAL",
			"THIS_FRAME"
		] call KH_fnc_execute;

		[
			{
				(CBA_missionTime > 0);
			},
			{				
				[
					{				
						[] call KH_fnc_serverMissionStartInit;
						[[], KH_fnc_headlessMissionStartInit, "HEADLESS", "THIS_FRAME"] call KH_fnc_execute;
						
						[
							[], 
							{
								systemChat "KH FRAMEWORK - MISSION STARTED";
							}, 
							"GLOBAL",
							"THIS_FRAME"
						] call KH_fnc_execute;
						
						[
							{
								private _initialPlayerCount = 0;

								{
									if (!(_x getUserInfo 7) && ((_x getUserInfo 6) > 8)) then {
										_initialPlayerCount = _initialPlayerCount + 1;
									};
								} forEach allUsers;
								
								if ((((count KH_var_allPlayerUnits) == _initialPlayerCount)) || (CBA_missionTime > 60) || !isMultiplayer) then {
									[
										{
											KH_var_initialPlayerUnits = KH_var_allPlayerUnits;
											publicVariable "KH_var_initialPlayerUnits";
											KH_var_playersLoaded = true;
											publicVariable "KH_var_playersLoaded";
											["KH_var_playersLoaded", KH_var_initialPlayerUnits] call CBA_fnc_globalEvent;												
											[] call KH_fnc_serverPlayersLoadedInit;
											[[], KH_fnc_playerPlayersLoadedInit, "PLAYERS", "THIS_FRAME"] call KH_fnc_execute;

											[
												[], 
												{
													systemChat "KH FRAMEWORK - PLAYERS INITIALIZED";
												}, 
												"GLOBAL",
												"THIS_FRAME"
											] call KH_fnc_execute;
										},
										[]
									] call CBA_fnc_execNextFrame;
									
									[_handle] call CBA_fnc_removePerFrameHandler;
								};
							}, 
							0, 
							[]
						] call CBA_fnc_addPerFrameHandler;
					}, 
					[]
				] call CBA_fnc_execNextFrame;
			},
			[]
		] call CBA_fnc_waitUntilAndExecute;
	};
	
	if hasInterface then {
		["KH_eve_playerPreloadedInitial", [clientOwner, profileName, profileNameSteam]] call CBA_fnc_serverEvent;
	};

	if (!isServer && !hasInterface) then {
		["KH_eve_headlessPreloaded", [clientOwner]] call CBA_fnc_globalEvent;
	};

	{
		_x params ["_arguments", "_function"];
		_arguments call _function;
	} forEach KH_var_postInitExecutions;
};

true;