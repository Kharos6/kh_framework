KH_var_currentPlayerUnit = objNull;
KH_var_missionLoaded = false;
KH_var_cachedFunctions = createHashMap;
KH_var_postInitExecutions = [];
KH_var_cbaEventHandlerStack = [];

KH_var_remoteExecCommandsMode = if (isNumber (missionConfigFile >> "CfgRemoteExec" >> "Commands" >> "mode")) then {
	getNumber (missionConfigFile >> "CfgRemoteExec" >> "Commands" >> "mode");
}
else {
	getNumber (configFile >> "CfgRemoteExec" >> "Commands" >> "mode");
};

KH_var_remoteExecFunctionsMode = if (isNumber (missionConfigFile >> "CfgRemoteExec" >> "Functions" >> "mode")) then {
	getNumber (missionConfigFile >> "CfgRemoteExec" >> "Functions" >> "mode");
}
else {
	getNumber (configFile >> "CfgRemoteExec" >> "Functions" >> "mode");
};

KH_var_remoteExecCommandsWhitelist = createHashMap;
KH_var_remoteExecFunctionsWhitelist = createHashMap;
KH_var_remoteExecCommandsJipWhitelist = createHashMap;
KH_var_remoteExecFunctionsJipWhitelist = createHashMap;

if (KH_var_remoteExecCommandsMode == 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) != 0) then {
				KH_var_remoteExecCommandsWhitelist set [toLower (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) != 0) then {
				KH_var_remoteExecCommandsJipWhitelist set [toLower (configName _x), true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Commands"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) != 0) then {
				KH_var_remoteExecCommandsWhitelist set [toLower (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) != 0) then {
				KH_var_remoteExecCommandsJipWhitelist set [toLower (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Commands"));
};

if (KH_var_remoteExecFunctionsMode == 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) != 0) then {
				KH_var_remoteExecCommandsWhitelist set [toLower (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) != 0) then {
				KH_var_remoteExecCommandsJipWhitelist set [toLower (configName _x), true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Functions"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) != 0) then {
				KH_var_remoteExecFunctionsWhitelist set [toLower (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) != 0) then {
				KH_var_remoteExecFunctionsJipWhitelist set [toLower (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Functions"));
};

["KH_eve_executionGlobal", KH_fnc_callParsedFunction] call CBA_fnc_addEventHandler;

if isServer then {
	KH_fnc_serverMissionStartInit = {};
	KH_fnc_serverMissionLoadInit = {};
	KH_fnc_serverPlayersLoadedInit = {};
	KH_fnc_serverMissionEndInit = {};
	KH_fnc_playerPlayersLoadedInit = {};
	KH_fnc_playerPreloadInit = {};
	KH_fnc_playerJipPreloadInit = {};
	KH_fnc_playerLoadInit = {};
	KH_fnc_playerJipLoadInit = {};
	KH_fnc_playerKilledInit = {};
	KH_fnc_playerSwitchInit = {};
	KH_fnc_playerRespawnInit = {};
	KH_fnc_playerMissionEndInit = {};
	KH_fnc_headlessMissionStartInit = {};
	KH_fnc_headlessLoadInit = {};
	KH_fnc_headlessMissionEndInit = {};
	KH_var_missionStarted = false;
	publicVariable "KH_var_missionStarted";
	KH_var_playersLoaded = false;
	publicVariable "KH_var_playersLoaded";
	KH_var_currentAdmin = 2;
	publicVariable "KH_var_currentAdmin";
	KH_var_disconnectedPlayers = [];
	publicVariable "KH_var_disconnectedPlayers";
	KH_var_allMachines = [2];
	publicVariable "KH_var_allMachines";
	KH_var_allCuratorMachines = [];
	publicVariable "KH_var_allCuratorMachines";
	KH_var_allHeadlessMachines = [];
	publicVariable "KH_var_allHeadlessMachines";
	KH_var_allHeadlessIdMachines = createHashMap;
	publicVariable "KH_var_allHeadlessIdMachines";
	KH_var_allPlayerMachines = [];
	publicVariable "KH_var_allPlayerMachines";
	KH_var_allPlayerIdMachines = createHashMap;
	publicVariable "KH_var_allPlayerIdMachines";
	KH_var_allPlayerUidMachines = createHashMap;
	publicVariable "KH_var_allPlayerUidMachines";
	KH_var_allPlayerUnits = [];
	publicVariable "KH_var_allPlayerUnits";
	KH_var_initialPlayerUnits = [];
	publicVariable "KH_var_initialPlayerUnits";
	KH_var_jipPlayerUnits = [];
	publicVariable "KH_var_jipPlayerUnits";
	KH_var_jipPlayerMachines = [];
	publicVariable "KH_var_jipPlayerMachines";
	KH_var_logicGroup = createGroup [sideLogic, false];
	publicVariable "KH_var_logicGroup";
	KH_var_jipEventHandlers = createHashMap;
	KH_var_allEntities = entities [[], ["Animal"], true, false];
	KH_var_allDeadEntities = (entities [[], ["Animal"], true, false]) select {!alive _x;};
	KH_var_allTerrainObjects = nearestTerrainObjects [[worldSize / 2, worldSize / 2], [], worldSize * sqrt 2 / 2, false, true];
	KH_var_headlessClientTransfers = [];
	KH_var_dynamicSimulationEntities = [];
	KH_var_entityArrayBuilderArrays = [];
	KH_var_groupArrayBuilderArrays = [];
	KH_var_initialSideRelations = [];
	["KH_eve_executionServer", KH_fnc_callParsedFunction] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerPreloadedInitial", 
		{
			params ["_machineId", "_profileName", "_profileNameSteam"];
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
					KH_var_allPlayerIdMachines insert [[_x getUserInfo 0, _machineId]];
					publicVariable "KH_var_allPlayerIdMachines";

					[
						[_uid], 
						{
							params ["_uid"];
							profileNamespace setVariable ["KH_var_steamId", _uid];
							missionNamespace setVariable ["KH_var_steamId", _uid];
						}, 
						_machineId, 
						"THIS_FRAME"
					] call KH_fnc_execute;

					break;
				};
			} forEach allUsers;
			
			if KH_var_playersLoaded then {
				[[], KH_fnc_playerJipPreloadInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
				KH_var_jipPlayerMachines pushBackUnique _machineId;
				publicVariable "KH_var_jipPlayerMachines";
				missionNamespace setVariable ["KH_var_isJip", true, _machineId];
			}
			else {
				[[], KH_fnc_playerPreloadInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			};

			if ((admin _machineId) != 0) then {
				KH_var_currentAdmin = _machineId;
				publicVariable "KH_var_currentAdmin";
			};

			["KH_eve_playerPreloaded", [_machineId, _uid, _profileName, _profileNameSteam]] call CBA_fnc_globalEvent;
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

			{
				if ((_x getUserInfo 1) == _machineId) then {
					KH_var_allHeadlessIdMachines insert [[_x getUserInfo 0, _machineId]];
					publicVariable "KH_var_allHeadlessIdMachines";
					break;
				};
			} forEach allUsers;

			[[], KH_fnc_headlessLoadInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerLoaded",
		{
			params ["_unit", "_machineId"];

			if KH_var_playersLoaded then {
				KH_var_jipPlayerUnits pushBackUnique _unit;
				publicVariable "KH_var_jipPlayerUnits";
				[[], KH_fnc_playerJipLoadInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			}
			else {
				[[], KH_fnc_playerLoadInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			};

			KH_var_allPlayerUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerUnits";
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
			params ["_newUnit", "_machineId", "_previousUnit"];
			[[_previousUnit], KH_fnc_playerSwitchInit, _machineId, "THIS_FRAME"] call KH_fnc_execute;
			
			if (_previousUnit in KH_var_allPlayerUnits) then {
				KH_var_allPlayerUnits deleteAt (KH_var_allPlayerUnits find _previousUnit);
				KH_var_allPlayerUnits pushBackUnique _newUnit;
				publicVariable "KH_var_allPlayerUnits";
			};

			if (_previousUnit in KH_var_jipPlayerUnits) then {
				KH_var_jipPlayerUnits deleteAt (KH_var_jipPlayerUnits find _previousUnit);
				KH_var_jipPlayerUnits pushBackUnique _newUnit;
				publicVariable "KH_var_jipPlayerUnits";
			};

			if (_previousUnit in KH_var_initialPlayerUnits) then {
				KH_var_initialPlayerUnits deleteAt (KH_var_initialPlayerUnits find _previousUnit);
				KH_var_initialPlayerUnits pushBackUnique _newUnit;
				publicVariable "KH_var_initialPlayerUnits";
			};
		}
	] call CBA_fnc_addEventHandler;

	{
		if ((_x getUserInfo 8) != 0) then {
			KH_var_currentAdmin = _x getUserInfo 1;
			publicVariable "KH_var_currentAdmin";
			break;
		};
	} forEach allUsers;

	addMissionEventHandler [
		"OnUserAdminStateChanged", 
		{
			params ["_networkId", "_loggedIn", "_votedIn"];
			private _machineId = _networkId getUserInfo 1;
			
			if (_loggedIn || _votedIn) then {
				KH_var_currentAdmin = _machineId;
			}
			else {
				KH_var_currentAdmin = 2;
			};

			publicVariable "KH_var_currentAdmin";
			["KH_eve_adminChanged", [_machineId, _loggedIn, _votedIn]] call CBA_fnc_globalEvent;
		}
	];

	addMissionEventHandler [
		"EntityRespawned",
		{
			params ["_newEntity", "_oldEntity"];

			[
				{
					params ["_newEntity", "_oldEntity"];
					["KH_eve_playerRespawned", [_newEntity, owner _newEntity, _oldEntity]] call CBA_fnc_globalEvent;
					private _index = KH_var_allPlayerUnits find _oldEntity;
					
					if (_oldEntity in KH_var_allPlayerUnits) then {
						KH_var_allPlayerUnits deleteAt (KH_var_allPlayerUnits find _oldEntity);
						KH_var_allPlayerUnits pushBackUnique _newEntity;
						publicVariable "KH_var_allPlayerUnits";
					};

					if (_oldEntity in KH_var_jipPlayerUnits) then {
						KH_var_jipPlayerUnits deleteAt (KH_var_jipPlayerUnits find _oldEntity);
						KH_var_jipPlayerUnits pushBackUnique _newEntity;
						publicVariable "KH_var_jipPlayerUnits";
					};

					if (_oldEntity in KH_var_initialPlayerUnits) then {
						KH_var_initialPlayerUnits deleteAt (KH_var_initialPlayerUnits find _oldEntity);
						KH_var_initialPlayerUnits pushBackUnique _newEntity;
						publicVariable "KH_var_initialPlayerUnits";
					};
				}, 
				[_newEntity, _oldEntity]
			] call CBA_fnc_execNextFrame;
		}
	];

	addMissionEventHandler [
		"EntityCreated", 
		{
			params ["_entity"];
			KH_var_allEntities pushBackUnique _entity;
		}
	];

	addMissionEventHandler [
		"EntityKilled", 
		{
			params ["_entity", "_killer", "_instigator"];

			if (isPlayer _entity) then {
				["KH_eve_playerKilled", [_entity, owner _entity, _killer, _instigator]] call CBA_fnc_globalEvent;
			};

			KH_var_allDeadEntities pushBackUnique _entity;
		}
	];

	addMissionEventHandler [
		"EntityDeleted", 
		{
			params ["_entity"];

			if (_entity in KH_var_allPlayerUnits) then {
				KH_var_allPlayerUnits deleteAt (KH_var_allPlayerUnits find _entity);
				publicVariable "KH_var_allPlayerUnits";				
			};

			if (_entity in KH_var_jipPlayerUnits) then {
				KH_var_jipPlayerUnits deleteAt (KH_var_jipPlayerUnits find _entity);
				publicVariable "KH_var_jipPlayerUnits";				
			};

			if (_entity in KH_var_initialPlayerUnits) then {
				KH_var_initialPlayerUnits deleteAt (KH_var_initialPlayerUnits find _entity);
				publicVariable "KH_var_initialPlayerUnits";				
			};
		}
	];

	addMissionEventHandler [
		"HandleDisconnect", 
		{
			params ["_unit", "_id", "_uid", "_name"];
			private _machineId = KH_var_allPlayerUidMachines get _uid;

			if !(isNil "_machineId") then {
				private _attributes = [];
				
				if (alive _unit) then {
					_attributes = [_unit] call KH_fnc_getUnitAttributes;
				};

				["KH_eve_playerDisconnected", [_unit, _machineId, _attributes, _uid, _name]] call CBA_fnc_globalEvent;
				KH_var_disconnectedPlayers pushBackUnique _uid;
				publicVariable "KH_var_disconnectedPlayers";
				
				if (_unit in KH_var_allPlayerUnits) then {
					KH_var_allPlayerUnits deleteAt (KH_var_allPlayerUnits find _unit);
					publicVariable "KH_var_allPlayerUnits";				
				};

				if (_unit in KH_var_jipPlayerUnits) then {
					KH_var_jipPlayerUnits deleteAt (KH_var_jipPlayerUnits find _unit);
					publicVariable "KH_var_jipPlayerUnits";				
				};

				if (_unit in KH_var_initialPlayerUnits) then {
					KH_var_initialPlayerUnits deleteAt (KH_var_initialPlayerUnits find _unit);
					publicVariable "KH_var_initialPlayerUnits";				
				};
						
				if (_machineId in KH_var_allMachines) then {
					KH_var_allMachines deleteAt (KH_var_allMachines find _machineId);
					publicVariable "KH_var_allMachines";	
				};

				if (_machineId in KH_var_allCuratorMachines) then {
					KH_var_allCuratorMachines deleteAt (KH_var_allCuratorMachines find _machineId);
					publicVariable "KH_var_allCuratorMachines";	
				};

				if (_machineId in KH_var_allPlayerMachines) then {
					KH_var_allPlayerMachines deleteAt (KH_var_allPlayerMachines find _machineId);
					publicVariable "KH_var_allPlayerMachines";
				};

				if (_machineId in KH_var_jipPlayerMachines) then {
					KH_var_jipPlayerMachines deleteAt (KH_var_jipPlayerMachines find _machineId);
					publicVariable "KH_var_jipPlayerMachines";
				};

				private _deletion = "";

				{
					if (_y == _machineId) then {
						_deletion = _x;
						break;
					};
				} forEach KH_var_allPlayerUidMachines;

				if (_deletion != "") then {
					KH_var_allPlayerUidMachines deleteAt _deletion;
					publicVariable "KH_var_allPlayerUidMachines";
				};

				_deletion = "";

				{
					if (_y == _machineId) then {
						_deletion = _x;
						break;
					};
				} forEach KH_var_allPlayerIdMachines;

				if (_deletion != "") then {
					KH_var_allPlayerIdMachines deleteAt _deletion;
					publicVariable "KH_var_allPlayerIdMachines";
				};
			}
			else {
				private _machineId = KH_var_allHeadlessIdMachines get _id;

				if !(isNil "_machineId") then {
					if (_machineId in KH_var_allMachines) then {
						KH_var_allMachines deleteAt (KH_var_allMachines find _machineId);
						publicVariable "KH_var_allMachines";	
					};

					if (_machineId in KH_var_allHeadlessMachines) then {
						KH_var_allHeadlessMachines deleteAt (KH_var_allHeadlessMachines find _machineId); 
						publicVariable "KH_var_allHeadlessMachines";
					};

					private _deletion = "";

					{
						if (_y == _machineId) then {
							_deletion = _x;
							break;
						};
					} forEach KH_var_allHeadlessIdMachines;

					if (_deletion != "") then {
						KH_var_allHeadlessIdMachines deleteAt _deletion;
						publicVariable "KH_var_allHeadlessIdMachines";
					};
				};
			};

			nil;
		}
	];
};

if hasInterface then {
	KH_var_isJip = false;
	KH_var_contextMenuOpen = false;
	KH_var_interactionMenuOpen = false;
	["KH_eve_executionPlayer", KH_fnc_callParsedFunction] call CBA_fnc_addEventHandler;

	[
		"unit", 
		{
			params ["_unit"];
			KH_var_currentPlayerUnit = _unit;
		},
		true
	] call CBA_fnc_addPlayerEventHandler;

	addMissionEventHandler [
		"PreloadFinished", 
		{
			if !(missionNamespace getVariable ["KH_var_playerWaiting", false]) then {
				missionNamespace setVariable ["KH_var_playerWaiting", true];

				[
					{				
						(!(isNull player) && (alive player));
					}, 
					{						
						[
							{
								["KH_eve_playerLoaded", [player, clientOwner]] call CBA_fnc_globalEvent;					
								missionNamespace setVariable ["KH_var_playerWaiting", false];

								(findDisplay 46) displayAddEventHandler [
									"KeyDown", 
									{
										private _key = _this select 1;
										private _shift = _this select 2;
										private _ctrl = _this select 3;
										private _alt = _this select 4;

										switch true do {
											case ((_key isEqualTo 0xDC) && !dialog && _alt && !_ctrl && !KH_var_interactionMenuOpen && (alive player)): {
												KH_var_interactionMenuOpen = true;
												private _display = [] call KH_fnc_openSelfInteractionMenu;

												[
													{
														params ["_display"];
														((isNull _display) || !(alive player) || (player getVariable ["ACE_isUnconscious", false]) || ((lifeState player) == "INCAPACITATED"));
													},
													{
														params ["_display"];
														KH_var_interactionMenuOpen = false;

														if !(isNull _display) then {
															_display closeDisplay 2;
														};
													}, 
													[_display]
												] call CBA_fnc_waitUntilAndExecute;
											};

											case ((_key isEqualTo 0xDC) && !dialog && !_alt && !_ctrl && !KH_var_interactionMenuOpen && (alive player)): {
												private _object = objNull;

												if !(isNull cursorObject) then {
													_object = cursorObject;
												}
												else {
													_object = cursorTarget;
												};

												if ((_object distance player) < 4) then {
													KH_var_interactionMenuOpen = true;
													private _display = [_object] call KH_fnc_openRemoteInteractionMenu;

													[
														{
															params ["_display", "_object"];
															((isNull _display) || ((_object distance player) > 4) || !(alive player) || (player getVariable ["ACE_isUnconscious", false]) || ((lifeState player) == "INCAPACITATED"));
														},
														{
															params ["_display"];
															KH_var_interactionMenuOpen = false;

															if !(isNull _display) then {
																_display closeDisplay 2;
															};
														}, 
														[_display, _object]
													] call CBA_fnc_waitUntilAndExecute;
												};
											};

											case ((_key isEqualTo 0xDC) && !dialog && _ctrl && _alt && !KH_var_interactionMenuOpen && (alive player)): {
												KH_var_interactionMenuOpen = true;
												private _display = [player] call KH_fnc_openSimulatedInventory;

												[
													{
														params ["_display"];
														((isNull _display) || !(alive player) || (player getVariable ["ACE_isUnconscious", false]) || ((lifeState player) == "INCAPACITATED"));
													},
													{
														params ["_display"];
														KH_var_interactionMenuOpen = false;

														if !(isNull _display) then {
															_display closeDisplay 2;
														};
													}, 
													[_display]
												] call CBA_fnc_waitUntilAndExecute;
											};

											case ((_key isEqualTo 0xDC) && !dialog && _ctrl && !_alt && !KH_var_interactionMenuOpen && (alive player)): {
												private _object = objNull;

												if !(isNull cursorObject) then {
													_object = cursorObject;
												}
												else {
													_object = cursorTarget;
												};

												if ((_object distance player) < 4) then {
													KH_var_interactionMenuOpen = true;
													private _display = [_object] call KH_fnc_openSimulatedInventory;

													[
														{
															params ["_display", "_object"];
															((isNull _display) || ((_object distance player) > 4) || !(alive player) || (player getVariable ["ACE_isUnconscious", false]) || ((lifeState player) == "INCAPACITATED"));
														},
														{
															params ["_display"];
															KH_var_interactionMenuOpen = false;

															if !(isNull _display) then {
																_display closeDisplay 2;
															};
														}, 
														[_display, _object]
													] call CBA_fnc_waitUntilAndExecute;
												};
											};
										};

										nil;
									}
								];
							}, 
							[]
						] call CBA_fnc_execNextFrame;
					},
					[]
				] call CBA_fnc_waitUntilAndExecute;
			};
		}
	];
	
	addMissionEventHandler [
		"TeamSwitch", 
		{
			params ["_previousUnit", "_newUnit"];

			[
				{
					params ["_previousUnit", "_newUnit"];
					["KH_eve_playerSwitched", [_newUnit, clientOwner, _previousUnit]] call CBA_fnc_globalEvent;
				}, 
				[_previousUnit, _newUnit]
			] call CBA_fnc_execNextFrame;
		}
	];
};

if (!isServer && !hasInterface) then {
	KH_var_isJip = false;
	["KH_eve_executionHeadless", KH_fnc_callParsedFunction] call CBA_fnc_addEventHandler;
};

true;