isNil {
	[
		"KH_eve_executionGlobal", 
		{
			params ["_arguments", "_function"];

			if (_function isEqualType "") then {
				_arguments call (missionNamespace getVariable [_function, {}]);
			}
			else {
				_arguments call _function;
			};
		}
	] call CBA_fnc_addEventHandler;

	if isServer then {
		[
			"KH_eve_executionServer", 
			{
				params ["_arguments", "_function"];

				if (_function isEqualType "") then {
					_arguments call (missionNamespace getVariable [_function, {}]);
				}
				else {
					_arguments call _function;
				};
			}
		] call CBA_fnc_addEventHandler;

		[
			"KH_eve_playerPreloadedInitial", 
			{
				params ["_machineId"];
				KH_var_allMachines pushBackUnique _machineId;
				publicVariable "KH_var_allMachines";
				KH_var_allPlayerMachines pushBackUnique _machineId;
				publicVariable "KH_var_allPlayerMachines";
				private _uid = "";
				
				{
					if ((_x getUserInfo 1) == _machineId) then {
						_uid = _x getUserInfo 2;
						KH_var_allPlayerUidMachines insert [[_uid, _machineId]];
						publicVariable "KH_var_allPlayerUidMachines";
						break;
					};
				} forEach allUsers;

				if ((admin _machineId) != 0) then {
					KH_var_currentAdmin = _machineId;
					publicVariable "KH_var_currentAdmin";
				};

				["KH_eve_playerPreloaded", [clientOwner, _uid]] call CBA_fnc_globalEvent;
				[[], KH_fnc_playerPreInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			}
		] call CBA_fnc_addEventHandler;

		[
			"KH_eve_headlessPreloaded", 
			{
				params ["_machineId"];
				KH_var_allMachines pushBackUnique _machineId;
				publicVariable "KH_var_allMachines";
				KH_var_allHeadlessMachines pushBackUnique _machineId;
				publicVariable "KH_var_allHeadlessMachines";
				[[], KH_fnc_headlessPreInit, "HEADLESS", "THIS_FRAME"] call KH_fnc_execute;
			}
		] call CBA_fnc_addEventHandler;
		
		[
			"KH_eve_playerLoaded",
			{
				params ["_unit", "_machineId"];
				KH_var_allPlayerUnits pushBackUnique _unit;
				publicVariable "KH_var_allPlayerUnits";
				[[], KH_fnc_playerPostInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			}
		] call CBA_fnc_addEventHandler;

		[
			"KH_eve_playerRespawned", 
			{
				params ["_unit", "_machineId", "_corpse"];
				[[_corpse], KH_fnc_playerRespawnInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			}
		] call CBA_fnc_addEventHandler;

		[
			"KH_eve_playerKilled", 
			{
				params ["_unit", "_machineId"];
				[[], KH_fnc_playerKilledInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			}
		] call CBA_fnc_addEventHandler;
		
		[
			"KH_eve_playerSwitched", 
			{
				params ["_newUnit", "_previousUnit", "_machineId"]:
				[[_previousUnit], KH_fnc_playerSwitchInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
				
				if (_previousUnit in KH_var_allPlayerUnits) then {
					KH_var_allPlayerUnits deleteAt (KH_var_allPlayerUnits find _previousUnit);
				};

				KH_var_allPlayerUnits pushBackUnique _newUnit;
				publicVariable "KH_var_allPlayerUnits";
			}
		] call CBA_fnc_addEventHandler;

		["KH_eve_missionInitialized", []] call CBA_fnc_globalEvent;	

		if (KH_var_entityArrayBuilderArrays isNotEqualTo []) then {
			{				
				missionNamespace setVariable [_x, missionNamespace getVariable [_x, []], true];

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

				addMissionEventHandler [
					"EntityDeleted", 
					{
						params ["_entity"];
						{
							private _entityArray = missionNamespace getVariable [_x, []];

							if (_entity in _entityArray) then {
								_entityArray deleteAt (_entityArray find _entity);
								missionNamespace setVariable [_x, _entityArray, true];
							};
						} forEach KH_var_entityArrayBuilderArrays;
					}
				];
			} forEach KH_var_entityArrayBuilderArrays;
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

				[
					{
						(((owner _headlessClient) in KH_var_allHeadlessMachines) && ((owner _headlessClient) != 2));
					},
					{
						params ["_assignedEntities", "_assignedEntitiesRecreate", "_headlessClient"];
						[_assignedEntities, owner _headlessClient, false, {}] call KH_fnc_setOwnership;
						[_assignedEntitiesRecreate, owner _headlessClient, true, {}] call KH_fnc_setOwnership;
					},
					[_assignedEntities, _assignedEntitiesRecreate, _headlessClient]
				] call CBA_fnc_waitUntilAndExecute;
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
	
		[] call KH_fnc_serverPreInit;
		
		[
			[], 
			{
				systemChat "KH FRAMEWORK PRE INIT COMPLETE";
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
						[] call KH_fnc_serverPostInit;
						[[], KH_fnc_headlessPostInit, "HEADLESS", "THIS_FRAME"] call KH_fnc_execute;
						
						[
							[], 
							{
								systemChat "KH FRAMEWORK POST INIT COMPLETE";
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
									KH_var_initialPlayerUnits = KH_var_allPlayerUnits;
									publicVariable "KH_var_initialPlayerUnits";
									KH_var_playersInitialized = true;
									publicVariable "KH_var_playersInitialized";
									["KH_eve_playersInitialized", []] call CBA_fnc_globalEvent;	

									[
										"KH_eve_playerLoaded",
										{
											private _machineId = _this select 1;
											[[], KH_fnc_playerJipInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
										}
									] call CBA_fnc_addEventHandler;
									
									[] call KH_fnc_serverMissionInit;
									[[], KH_fnc_headlessMissionInit, "HEADLESS", "THIS_FRAME"] call KH_fnc_execute;
									[[], KH_fnc_playerMissionInit, "PLAYERS", "THIS_FRAME"] call KH_fnc_execute;

									[
										[], 
										{
											systemChat "KH FRAMEWORK MISSION INIT COMPLETE";
										}, 
										"GLOBAL",
										"THIS_FRAME"
									] call KH_fnc_execute;
									
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
		[
			"KH_eve_executionPlayer", 
			{
				params ["_arguments", "_function"];

				if (_function isEqualType "") then {
					_arguments call (missionNamespace getVariable [_function, {}]);
				}
				else {
					_arguments call _function;
				};
			}
		] call CBA_fnc_addEventHandler;

		["KH_eve_playerPreloadedInitial", [clientOwner]] call CBA_fnc_serverEvent;
	};

	if (!isServer && !hasInterface) then {
		[
			"KH_eve_executionHeadless",
			{
				params ["_arguments", "_function"];

				if (_function isEqualType "") then {
					_arguments call (missionNamespace getVariable [_function, {}]);
				}
				else {
					_arguments call _function;
				};
			}
		] call CBA_fnc_addEventHandler;

		["KH_eve_headlessPreloaded", [clientOwner]] call CBA_fnc_globalEvent;
	};
};

true;