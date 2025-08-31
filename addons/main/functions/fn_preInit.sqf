KH_var_gameSessionId = uiNamespace getVariable "KH_var_gameSessionId";
KH_var_missionSessionId = call KH_fnc_generateUid;
uiNamespace setVariable ["KH_var_missionSessionId", KH_var_missionSessionId];
KH_var_aceLoaded = uiNamespace getVariable "KH_var_aceLoaded";
KH_var_missionLoaded = false;
KH_var_jip = false;
KH_var_playerUnit = objNull;
KH_var_cachedLuaFunctions = createHashMap;
KH_var_cachedLuaEntities = createHashMap;
KH_var_remoteExecCommandsBlacklist = createHashMap;
KH_var_remoteExecFunctionsBlacklist = createHashMap;
KH_var_remoteExecCommandsJipBlacklist = createHashMap;
KH_var_remoteExecFunctionsJipBlacklist = createHashMap;
KH_var_cbaEventHandlerStack = createHashMap;
KH_var_playerEventHandlerStack = createHashMap;
KH_var_inGameUiEventHandlerStack = createHashMap;
KH_var_temporalExecutionStackMonitor = createHashMap;
KH_var_drawUiExecutionStackMonitor = createHashMap;
KH_var_cbaEventHandlerStackDeletions = [];
KH_var_playerEventHandlerStackDeletions = [];
KH_var_inGameUiEventHandlerStackDeletions = [];
KH_var_temporalExecutionStack = [];
KH_var_drawUi2dExecutionStack = [];
KH_var_drawUi3dExecutionStack = [];
KH_var_temporalExecutionStackAdditions = [];
KH_var_drawUi2dExecutionStackAdditions = [];
KH_var_drawUi3dExecutionStackAdditions = [];
KH_var_temporalExecutionStackDeletions = [];
KH_var_drawUi2dExecutionStackDeletions = [];
KH_var_drawUi3dExecutionStackDeletions = [];
KH_var_postInitExecutions = [];
KH_var_preInitLuaExecutions = [];
KH_var_postInitLuaExecutions = [];
KH_var_loadInitLuaExecutions = [];
call KH_fnc_luaClearVariables;
call KH_fnc_luaResetState;

{
    private _config = _x;
    private _prefix = getText (_config >> "prefix");

    {
        private _function = if (isText (_x >> "path")) then {
            preprocessFile ([(getText (_x >> "path")), "\", configName _x, ".lua"] joinString "");
        }
        else {
            preprocessFile ([(getText (_config >> "path")), "\", configName _x, ".lua"] joinString "");
        };

        private _name = if (isText (_x >> "name")) then {
            getText (_x >> "name");
        }
        else {
           	[_prefix, "lua", configName _x] joinString "_";
        };

        if (isNil {[_function, _name] call KH_fnc_luaCompile;}) then {
			continue;
		};

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
        		KH_var_preInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
        		KH_var_postInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "loadInit")) then {
			if ((getNumber (_x >> "loadInit")) isEqualTo 1) then {
        		KH_var_loadInitLuaExecutions pushBack _function;
			};
        };
    } forEach ("true" configClasses _config);
} forEach ("true" configClasses (configFile >> "CfgLuaFunctions"));

{
    private _config = _x;
    private _prefix = getText (_config >> "prefix");

    {
        private _function = if (isText (_x >> "path")) then {
            preprocessFile ([(getText (_x >> "path")), "\", configName _x, ".lua"] joinString "");
        }
        else {
            preprocessFile ([(getText (_config >> "path")), "\", configName _x, ".lua"] joinString "");
        };

        private _name = if (isText (_x >> "name")) then {
            getText (_x >> "name");
        }
        else {
           	[_prefix, "lua", configName _x] joinString "_";
        };

        if (isNil {[_function, _name] call KH_fnc_luaCompile;}) then {
			continue;
		};

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
        		KH_var_preInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
        		KH_var_postInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "loadInit")) then {
			if ((getNumber (_x >> "loadInit")) isEqualTo 1) then {
        		KH_var_loadInitLuaExecutions pushBack _function;
			};
        };
    } forEach ("true" configClasses _config);
} forEach ("true" configClasses (missionConfigFile >> "CfgLuaFunctions"));

uiNamespace setVariable ["KH_var_cachedLuaFunctions", KH_var_cachedLuaFunctions];

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

KH_var_remoteExecCommandsJipMode = if (isNumber (missionConfigFile >> "CfgRemoteExec" >> "Commands" >> "jip")) then {
	getNumber (missionConfigFile >> "CfgRemoteExec" >> "Commands" >> "mode");
}
else {
	getNumber (configFile >> "CfgRemoteExec" >> "Commands" >> "mode");
};

KH_var_remoteExecFunctionsJipMode = if (isNumber (missionConfigFile >> "CfgRemoteExec" >> "Functions" >> "jip")) then {
	getNumber (missionConfigFile >> "CfgRemoteExec" >> "Functions" >> "mode");
}
else {
	getNumber (configFile >> "CfgRemoteExec" >> "Functions" >> "mode");
};

if (KH_var_remoteExecCommandsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLower (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLower (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Commands"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLower (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJippBlacklist set [toLower (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Commands"));
};

if (KH_var_remoteExecFunctionsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandsBlacklist set [toLower (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLower (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Functions"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecFunctionsBlacklist set [toLower (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecFunctionsJipBlacklist set [toLower (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Functions"));
};

addMissionEventHandler [
	"ExtensionCallback",
	{
		params ["_name", "_function", "_data"];

		if (_name isEqualTo "kh_framework") then {
			private _args = parseSimpleArray _data;
			private _parsedArgs = [];

			{
				if (_x isEqualType "") then {
					if ("KH_var_cachedLuaEntity_" in _x) then {
						_parsedArgs pushBack (KH_var_cachedLuaEntities get _x);
					}
					else {
						_parsedArgs pushBack _x;
					};
				}
				else {
					_parsedArgs pushBack _x;
				};
			} forEach _args;

			[_parsedArgs, [_function, false] call KH_fnc_parseFunction, clientOwner] call KH_fnc_callParsedFunction;
		};
	}
];

["KH_eve_execution", KH_fnc_callParsedFunction] call CBA_fnc_addEventHandler;

[
	"KH_eve_registerCallback", 
	{
		params ["_arguments", ["_function", "", [""]], ["_caller", 2, [0]], ["_id", "", [""]]];
		missionNamespace setVariable [_id, [_arguments, _function, _caller] call KH_fnc_callParsedFunction, _caller];		
	}
] call CBA_fnc_addEventHandler;

{
	[[isServer, hasInterface], _x] call KH_fnc_luaOperation;
} forEach KH_var_preInitLuaExecutions;

addMissionEventHandler [
	"EachFrame", 
	{
		if (KH_var_temporalExecutionStackDeletions isNotEqualTo []) then {
			{
				if ((_x select 6) in KH_var_temporalExecutionStackDeletions) then {
					KH_var_temporalExecutionStackDeletions set [KH_var_temporalExecutionStackDeletions find (_x select 6), _forEachIndex];
				};
			} forEach KH_var_temporalExecutionStack;

			KH_var_temporalExecutionStack deleteAt (KH_var_temporalExecutionStackDeletions select {_x isEqualType 0;});
			KH_var_temporalExecutionStackDeletions = [];
		};

		if (KH_var_temporalExecutionStackAdditions isNotEqualTo []) then {
			KH_var_temporalExecutionStack append KH_var_temporalExecutionStackAdditions;
		};

		{
			_x params ["_args", "_function", "_delay", "_delta", "_totalDelta", "_eventId", "_eventName", "_previousReturn"];

			if (_eventName in KH_var_temporalExecutionStackDeletions) then {
				continue;
			};

			if (_delay isEqualTo 0) then {
				_totalDelta = if (_totalDelta isEqualTo -1) then {
					diag_deltaTime;
				}
				else {
					_x set [4, systemTime joinString ""];
					[[systemTime joinString "", " - ", _totalDelta] joinString ""] call KH_fnc_mathOperation;
				};

				_x set [7, _args call _function];
				continue;
			};

			if (_delay > 0) then {
				if (diag_tickTime >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, systemTime joinString ""];
						[[systemTime joinString "", " - ", _totalDelta] joinString ""] call KH_fnc_mathOperation;
					};

					_x set [7, _args call _function];
					_x set [3, _delta + _delay];
				};
			}
			else {
				if (diag_frameNo >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, systemTime joinString ""];
						[[systemTime joinString "", " - ", _totalDelta] joinString ""] call KH_fnc_mathOperation;
					};

					_x set [7, _args call _function];
					_x set [3, _delta + (abs _delay)];
				};
			};
		} forEach KH_var_temporalExecutionStack;
	}
];

[
	"KH_eve_temporalExecutionStackHandler", 
	{
		params ["_handler", "_deleteHandler", "_overrideTimeoutOnDeletion"];
		private _currentHandler = KH_var_temporalExecutionStackMonitor get _handler;
		if (isNil "_currentHandler") exitWith {};
		_currentHandler params ["_timeoutArguments", "_timeoutFunction", "_handlerTickCounter", "_timeout", "_timeoutPriority", "_timeoutOnDeletion"];

		if _deleteHandler exitWith {
			if !(_handler in KH_var_temporalExecutionStackDeletions) then {

				KH_var_temporalExecutionStackDeletions insert [
					if _timeoutPriority then {
						0;
					}
					else {
						-1;
					}, 
					[_handler]
				];

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handler;
		};

		if ((missionNamespace getVariable [_handlerTickCounter, 1]) >= _timeout) then {
			if !(_handler in KH_var_temporalExecutionStackDeletions) then {

				KH_var_temporalExecutionStackDeletions insert [
					if _timeoutPriority then {
						0;
					}
					else {
						-1;
					}, 
					[_handler]
				];

				_timeoutArguments call _timeoutFunction;
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handler;
		}
		else {
			missionNamespace setVariable [_handlerTickCounter, (missionNamespace getVariable [_handlerTickCounter, 1]) + 1];
		};
	}
] call CBA_fnc_addEventHandler;

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
	KH_var_adminMachine = 2;
	publicVariable "KH_var_adminMachine";
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
	KH_var_initialPlayerMachines = [];
	publicVariable "KH_var_initialPlayerMachines";
	KH_var_jipPlayerUnits = [];
	publicVariable "KH_var_jipPlayerUnits";
	KH_var_jipPlayerMachines = [];
	publicVariable "KH_var_jipPlayerMachines";
	KH_var_logicGroup = createGroup [sideLogic, false];
	publicVariable "KH_var_logicGroup";
	KH_var_jipHandlers = createHashMap;
	KH_var_allEntities = entities [[], ["Animal"], true, false];
	KH_var_allDeadEntities = (entities [[], ["Animal"], true, false]) select {!alive _x;};
	KH_var_allTerrainObjects = nearestTerrainObjects [[worldSize / 2, worldSize / 2], [], worldSize * sqrt 2 / 2, false, true];
	KH_var_headlessClientTransfers = [];
	KH_var_dynamicSimulationEntities = [];
	KH_var_entityArrayBuilderArrays = [];
	KH_var_groupArrayBuilderArrays = [];
	KH_var_initialSideRelations = [];

	[
		"KH_eve_jipSetup",
		{
			params ["_name", "_arguments", "_dependency", "_unitRequired", "_jipId"];
			missionNamespace setVariable [_jipId, true];
			private _currentHandler = KH_var_jipHandlers get _jipId;
			private _continue = true;

			if !(isNil "_currentHandler") then {
				if ((_currentHandler select 4) isEqualTo _unitRequired) then {
					KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, []]];
					_continue = false;
				}
				else {
					{
						[_x] call KH_fnc_removeEventHandler;
					} forEach (_currentHandler select 5);
				};
			};

			if !_continue exitWith {};
			private _jipHandlers = [];
			KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, _jipHandlers]];
			
			private _joinType = if _unitRequired then {
				["KH_eve_playerLoaded", "KH_eve_headlessPreloaded"];
			}
			else {
				["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
			};

			{
				private _jipHandler = [
					"CBA",
					_x,
					[_jipId, _x],
					{
						params ["_joiningMachine", "_uid"];
						_args params ["_jipId", "_joinType"];
						
						if (missionNamespace getVariable _jipId) then {
							private _currentHandler = KH_var_jipHandlers get _jipId;
							if (isNil "_currentHandler") exitWith {};
							private _dependency = _currentHandler select 3;
							private _condition = true;

							switch (typeName _dependency) do {
								case "BOOL": {
									if !_dependency then {
										_condition = false;
									};
								};

								case "SCALAR": {
									if !(_dependency in KH_var_allMachines) then {
										_condition = false;
									};
								};

								case "OBJECT": {
									if (isNull _dependency) then {
										_condition = false;
									};
								};

								case "TEAM_MEMBER": {
									if (isNull _dependency) then {
										_condition = false;
									};
								};

								case "GROUP": {
									if (isNull _dependency) then {
										_condition = false;
									};
								};

								case "STRING": {
									if ((parseNumber (_dependency select [0, 1])) isNotEqualTo 0) then {
										if (_dependency isNotEqualTo _uid) then {
											if (":" in _dependency) then {
												if ((isNull (objectFromNetId _dependency)) && (isNull (groupFromNetId _dependency))) then {
													_condition = false;
												};
											}
											else {
												private _player = KH_var_allPlayerIdMachines get _x;
												private _headlessClient = KH_var_allHeadlessIdMachines get _x;

												if ((isNil "_player") && (isNil "_headlessClient")) then {
													_condition = false;
												};
											};
										};
									}
									else {
										if !(missionNamespace getVariable [_dependency, false]) then {
											_condition = false;
										};
									};
								};

								case "ARRAY": {																					
									{
										switch (typeName _x) do {
											case "SCALAR": {
												if !(_x in KH_var_allMachines) then {
													_condition = false;
													break;
												};
											};

											case "OBJECT": {
												if (isNull _x) then {
													_condition = false;
													break;
												};
											};

											case "TEAM_MEMBER": {
												if (isNull _x) then {
													_condition = false;
													break;
												};
											};

											case "GROUP": {
												if (isNull _x) then {
													_condition = false;
													break;
												};
											};

											case "STRING": {
												if ((parseNumber (_x select [0, 1])) isNotEqualTo 0) then {
													if (_x isNotEqualTo _uid) then {
														if (":" in _x) then {
															if ((isNull (objectFromNetId _x)) && (isNull (groupFromNetId _x))) then {
																_condition = false;
																break;
															};
														}
														else {
															private _player = KH_var_allPlayerIdMachines get _x;
															private _headlessClient = KH_var_allHeadlessIdMachines get _x;

															if ((isNil "_player") && (isNil "_headlessClient")) then {
																_condition = false;
																break;
															};
														};
													};
												}
												else {
													if !(missionNamespace getVariable [_x, false]) then {
														_condition = false;
														break;
													};
												};
											};

											case "CODE": {
												if !(call _x) then {
													_condition = false;
													break;
												};
											};
										};
									} forEach _dependency;
								};

								case "CODE": {
									if !(call _dependency) then {
										_condition = false;
									};
								};
							};

							if _condition then {
								[_currentHandler select 1, _currentHandler select 2, _joiningMachine] call CBA_fnc_ownerEvent;
							};
						}
						else {
							KH_var_jipHandlers deleteAt _jipId;
							[_eventId] call KH_fnc_removeEventHandler;
						};
					}
				] call KH_fnc_addEventHandler;

				_jipHandlers pushBack _jipHandler;
			} forEach _joinType;
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_persistentExecutionSetup",
		{
			params ["_arguments", "_function", "_target", "_sendoffArguments", "_sendoffFunction", "_persistentExecutionId", "_caller"];
			_target setVariable [_persistentExecutionId, true, true];
			private _persistentEventId = [hashValue _target, _persistentExecutionId] joinString "";

			if (isNil {missionNamespace getVariable _persistentEventId;}) then {
				[
					"KH_eve_execution",
					[
						[
							["STANDARD", _target, "LOCAL"],
							"Local",
							[_persistentExecutionId, _persistentEventId],
							{
								params ["_entity", "_local"];
								_args params ["_persistentExecutionId", "_persistentEventId"];

								if (_entity getVariable _persistentExecutionId) then {
									(missionNamespace getVariable _persistentEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller"];
									
									if _local then {
										[_arguments, _function, _caller] call KH_fnc_callParsedFunction;
									}
									else {
										[_sendoffArguments, _sendoffFunction, _caller] call KH_fnc_callParsedFunction;
									};
								};
							}
						], 
						"KH_fnc_addEventHandler", 
						_caller
					],
					"GLOBAL",
					[_target, false, _persistentExecutionId]
				] call KH_fnc_triggerCbaEvent;
			};

			missionNamespace setVariable [_persistentEventId, [_arguments, _function, _sendoffArguments, _sendoffFunction, _caller], true];
		}
	] call CBA_fnc_addEventHandler;

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
						[_uid, _x getUserInfo 0], 
						{
							params ["_uid", "_directPlayId"];
							missionNamespace setVariable ["KH_var_steamId", _uid];
							missionNamespace setVariable ["KH_var_directPlayId", _directPlayId];
							missionNamespace setVariable ["KH_initComplete", true];
						}, 
						_machineId, 
						true
					] call KH_fnc_execute;

					break;
				};
			} forEach allUsers;
			
			if KH_var_playersLoaded then {
				[[], KH_fnc_playerJipPreloadInit, _machineId, true] call KH_fnc_execute;
				KH_var_jipPlayerMachines pushBackUnique _machineId;
				publicVariable "KH_var_jipPlayerMachines";
				missionNamespace setVariable ["KH_var_jip", true, _machineId];
			}
			else {
				[[], KH_fnc_playerPreloadInit, _machineId, true] call KH_fnc_execute;
			};

			if ((admin _machineId) != 0) then {
				KH_var_adminMachine = _machineId;
				publicVariable "KH_var_adminMachine";
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
					missionNamespace setVariable ["KH_initComplete", true, _machineId];
					publicVariable "KH_var_allHeadlessIdMachines";
					break;
				};
			} forEach allUsers;

			[[], KH_fnc_headlessLoadInit, _machineId, true] call KH_fnc_execute;
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerLoaded",
		{
			params ["_machineId", "_unit"];

			if KH_var_playersLoaded then {
				KH_var_jipPlayerUnits pushBackUnique _unit;
				publicVariable "KH_var_jipPlayerUnits";
				[[], KH_fnc_playerJipLoadInit, _machineId, true] call KH_fnc_execute;
			}
			else {
				[[], KH_fnc_playerLoadInit, _machineId, true] call KH_fnc_execute;
			};

			KH_var_allPlayerUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerUnits";
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerRespawned", 
		{
			params ["_unit", "_machineId", "_corpse"];
			[[_corpse], KH_fnc_playerRespawnInit, _machineId, true] call KH_fnc_execute;
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerKilled", 
		{
			params ["_unit", "_machineId"];
			[[], KH_fnc_playerKilledInit, _machineId, true] call KH_fnc_execute;
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerSwitched", 
		{
			params ["_newUnit", "_machineId", "_previousUnit"];
			[[_previousUnit], KH_fnc_playerSwitchInit, _machineId, true] call KH_fnc_execute;
			
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
			KH_var_adminMachine = _x getUserInfo 1;
			publicVariable "KH_var_adminMachine";
			break;
		};
	} forEach allUsers;

	addMissionEventHandler [
		"OnUserAdminStateChanged", 
		{
			params ["_networkId", "_loggedIn", "_votedIn"];
			private _machineId = _networkId getUserInfo 1;
			
			if (_loggedIn || _votedIn) then {
				KH_var_adminMachine = _machineId;
			}
			else {
				KH_var_adminMachine = 2;
			};

			publicVariable "KH_var_adminMachine";
			["KH_eve_adminChanged", [_machineId, _loggedIn, _votedIn]] call CBA_fnc_globalEvent;
		}
	];

	addMissionEventHandler [
		"EntityRespawned",
		{
			params ["_newEntity", "_oldEntity"];
			private _variableName = [_oldEntity, false] call KH_fnc_getEntityVariableName;
			_oldEntity setVehicleVarName "";

			[
				[_oldEntity],
				{
					params ["_oldEntity"];
					_oldEntity setVehicleVarName "";
				},
				"GLOBAL",
				true,
				true
			] call KH_fnc_execute;

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
	KH_var_contextMenuOpen = false;
	KH_var_interactionMenuOpen = false;

	[
		"KH Framework", 
		"KH_openInteractionMenu", 
		"Open Interaction Menu", 
		{
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
		}, 
		{}, 
		[0xDC, [false, false, true]]
	] call CBA_fnc_addKeybind;

	[
		"KH Framework", 
		"KH_openRemoteInteractionMenu", 
		"Open Remote Interaction Menu", 
		{
			private _object = if !(isNull cursorObject) then {
				cursorObject;
			}
			else {
				cursorTarget;
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
		}, 
		{}, 
		[0xDC, [false, false, false]]
	] call CBA_fnc_addKeybind;

	[
		"KH Framework", 
		"KH_openVirtualInventory", 
		"Open Virtual Inventory", 
		{
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
		}, 
		{}, 
		[0xDC, [false, true, true]]
	] call CBA_fnc_addKeybind;

	[
		"KH Framework", 
		"KH_openRemoteVirtualInventory", 
		"Open Remote Virtual Inventory", 
		{
			private _object = if !(isNull cursorObject) then {
				cursorObject;
			}
			else {
				cursorTarget;
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
		}, 
		{}, 
		[0xDC, [false, true, false]]
	] call CBA_fnc_addKeybind;

	[
		"unit", 
		{
			params ["_unit"];
			KH_var_playerUnit = _unit;
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
						["KH_eve_playerLoaded", [clientOwner, player, [player, true] call KH_fnc_getEntityVariableName]] call CBA_fnc_globalEvent;				
						missionNamespace setVariable ["KH_var_playerWaiting", false];

						{
							[[isServer, hasInterface], _x] call KH_fnc_luaOperation;
						} forEach KH_var_loadInitLuaExecutions;
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

	addMissionEventHandler [
		"Draw2D", 
		{
			if (KH_var_drawUi2dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi2dExecutionStackDeletions) then {
						KH_var_drawUi2dExecutionStackDeletions set [KH_var_drawUi2dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi2dExecutionStack;

				KH_var_drawUi2dExecutionStack deleteAt (KH_var_drawUi2dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi2dExecutionStackDeletions = [];
			};

			if (KH_var_drawUi2dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi2dExecutionStack append KH_var_drawUi2dExecutionStackAdditions;
			};

			{
				_x params ["_args", "_function", "_eventId", "_eventName", "_previousReturn"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi2dExecutionStack;
		}
	];

	addMissionEventHandler [
		"Draw3D", 
		{
			if (KH_var_drawUi3dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi3dExecutionStackDeletions) then {
						KH_var_drawUi3dExecutionStackDeletions set [KH_var_drawUi3dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi3dExecutionStack;

				KH_var_drawUi3dExecutionStack deleteAt (KH_var_drawUi3dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi3dExecutionStackDeletions = [];
			};

			if (KH_var_drawUi3dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi3dExecutionStack append KH_var_drawUi3dExecutionStackAdditions;
			};

			{
				_x params ["_args", "_function", "_eventId", "_eventName", "_previousReturn"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi3dExecutionStack;
		}
	];

	[
		"KH_eve_drawUiExecutionStackHandler", 
		{
			params ["_handler", "_overrideTimeoutOnDeletion"];
			private _currentHandler = KH_var_drawUiExecutionStackMonitor get _handler;
			if (isNil "_currentHandler") exitWith {};
			_currentHandler params ["_event", "_timeoutArguments", "_timeoutFunction", "_timeoutOnDeletion"];

			private _deletionType = switch _event do {
				case "2D": {
					KH_var_drawUi2dExecutionStackDeletions;
				};

				case "3D": {
					KH_var_drawUi3dExecutionStackDeletions;
				};
			};

			if !(_handler in _deletionType) then {
				_deletionType insert [0, [_handler]];

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			KH_var_drawUiExecutionStackMonitor deleteAt _handler;
		}
	] call CBA_fnc_addEventHandler;
};

nil;