KH_var_gameSessionId = uiNamespace getVariable "KH_var_gameSessionId";
KH_var_missionSessionId = generateUid;
uiNamespace setVariable ["KH_var_missionSessionId", KH_var_missionSessionId];
KH_var_aceLoaded = uiNamespace getVariable "KH_var_aceLoaded";
KH_var_missionLoaded = false;
KH_var_jip = false;
KH_var_playerUnit = objNull;
KH_var_defaultAiSystemPrompt = uiNamespace getVariable "KH_var_defaultAiSystemPrompt";
KH_var_remoteExecCommandsBlacklist = createHashMap;
KH_var_remoteExecFunctionsBlacklist = createHashMap;
KH_var_remoteExecCommandsJipBlacklist = createHashMap;
KH_var_remoteExecFunctionsJipBlacklist = createHashMap;
KH_var_inGameUiEventHandlerStack = createHashMap;
KH_var_temporalExecutionStackMonitor = createHashMap;
KH_var_drawUiExecutionStackMonitor = createHashMap;
KH_var_quickFunctionsSqf = createHashMap;
uiNamespace setVariable ["KH_var_quickFunctionsSqf", KH_var_quickFunctionsSqf];
KH_var_quickFunctionsLua = createHashMap;
uiNamespace setVariable ["KH_var_quickFunctionsLua", KH_var_quickFunctionsLua];
KH_var_inGameUiEventHandlerStackDeletions = [];
KH_var_temporalExecutionStack = [];
KH_var_drawUi2dExecutionStack = [];
KH_var_drawUi3dExecutionStack = [];
KH_var_drawUi3dOrphanExecutionStack = [];
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
KH_var_mouseTargetCheckFrame = 0;
KH_var_viewTargetCheckFrame = 0;
KH_var_weaponTargetCheckFrame = 0;
KH_var_allAddedDisplays = [];
KH_var_playerRespawnedEventHandler = [];
KH_var_playerKilledEventHandler = [];
["KH_eve_execution", KH_fnc_callSerializedFunction] call CBA_fnc_addEventHandler;

[
	"KH_eve_luaEventTrigger", 
	{
		params [["_event", "", [""]], "_arguments"];
		luaTriggerEvent _event;
	}
] call CBA_fnc_addEventHandler;

[
	"KH_eve_luaVariableEmission", 
	{
		params [["_name", "", [""]], "_value"];
		luaSetVariable [_name, _value];
	}
] call CBA_fnc_addEventHandler;

[
	"KH_eve_khDataWriteEmission", 
	{
		params [["_filename", "", [""]], ["_name", "", [""]], "_value"];
		_filename writeKhData [_name, _value];
	}
] call CBA_fnc_addEventHandler;

{
    private _config = _x;
    private _prefix = getText (_config >> "prefix");
    private _basePath = (getText (_config >> "path")) regexReplace ["(/)", "\\"];

    {
        private _pathUsed = isText (_x >> "path");

        private _function = preprocessFile ([
            _basePath,
            ["", "\"] select (_basePath isNotEqualTo ""),
            if _pathUsed then {
                (getText (_x >> "path")) regexReplace ["(/)", "\\"];
            }
            else {
                "";
            }, 
            configName _x, 
            ".lua"
        ] joinString "");

        private _name = if (isText (_x >> "name")) then {
            [_prefix, getText (_x >> "name")] joinString "_";
        }
        else {
           	[_prefix, configName _x] joinString "_";
        };

		if (isNumber (_x >> "compileSqf")) then {
			if ((getNumber (_x >> "compileSqf")) isEqualTo 1) then {
				private _compiled = [
                    [
                        _prefix,
                        "_fnc_",
                        if (isText (_x >> "name")) then {
                            getText (_x >> "name");
                        }
                        else {
                            configName _x;
                        }
                    ] joinString "",
                    compile (["_this luaExecute '", _name, "';"] joinString "")
				];

                missionNamespace setVariable _compiled;
				uiNamespace setVariable _compiled;
            };
        };

        _name luaCompile _function;

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
				if !(_name in KH_var_preInitLuaExecutions) then {
        			KH_var_preInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
				if !(_name in KH_var_postInitLuaExecutions) then {
        			KH_var_postInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "loadInit")) then {
			if ((getNumber (_x >> "loadInit")) isEqualTo 1) then {
				if !(_name in KH_var_loadInitLuaExecutions) then {
        			KH_var_loadInitLuaExecutions pushBack _name;
                };
			};
        };
    } forEach ("true" configClasses _config);
} forEach (("true" configClasses (configFile >> "CfgLuaFunctions")) + ("true" configClasses (missionConfigFile >> "CfgLuaFunctions")));

[
	"KH_eve_luaReset",
	{
		with uiNamespace do {
			{
				private _config = _x;
				private _prefix = getText (_config >> "prefix");
				private _basePath = (getText (_config >> "path")) regexReplace ["(/)", "\\"];

				{
					private _pathUsed = isText (_x >> "path");

					private _function = preprocessFile ([
						_basePath,
						["", "\"] select (_basePath isNotEqualTo ""),
						if _pathUsed then {
							(getText (_x >> "path")) regexReplace ["(/)", "\\"];
						}
						else {
							"";
						}, 
						configName _x, 
						".lua"
					] joinString "");

					private _name = if (isText (_x >> "name")) then {
						[_prefix, getText (_x >> "name")] joinString "_";
					}
					else {
						[_prefix, configName _x] joinString "_";
					};

					_name luaCompile _function;

					{
						luaExecute _x;
					} forEach KH_var_resetInitLuaExecutions;
				} forEach ("true" configClasses _config);
			} forEach ("true" configClasses (configFile >> "CfgLuaFunctions"));
		};
	}
] call CBA_fnc_addEventHandler;

KH_var_remoteExecCommandsMode = ["SCALAR", ["'CfgRemoteExec' >> 'Commands' >> 'mode'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecFunctionsMode = ["SCALAR", ["'CfgRemoteExec' >> 'Functions' >> 'mode'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecCommandsJipMode = ["SCALAR", ["'CfgRemoteExec' >> 'Commands' >> 'jip'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecFunctionsJipMode = ["SCALAR", ["'CfgRemoteExec' >> 'Functions' >> 'jip'", true]] call KH_fnc_getConfigValue;

if (KH_var_remoteExecCommandsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLowerANSI (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLowerANSI (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Commands"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJippBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Commands"));
};

if (KH_var_remoteExecFunctionsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandsBlacklist set [toLowerANSI (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLowerANSI (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Functions"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecFunctionsBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecFunctionsJipBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Functions"));
};

addMissionEventHandler [
	"EachFrame", 
	{
		if (KH_var_temporalExecutionStackAdditions isNotEqualTo []) then {
			KH_var_temporalExecutionStack append KH_var_temporalExecutionStackAdditions;
			KH_var_temporalExecutionStackAdditions resize 0;
		};

		if (KH_var_temporalExecutionStackDeletions isNotEqualTo []) then {
			{
				if ((_x select 6) in KH_var_temporalExecutionStackDeletions) then {
					KH_var_temporalExecutionStackDeletions set [KH_var_temporalExecutionStackDeletions find (_x select 6), _forEachIndex];
				};
			} forEach KH_var_temporalExecutionStack;

			KH_var_temporalExecutionStack deleteAt (KH_var_temporalExecutionStackDeletions select {_x isEqualType 0;});
			KH_var_temporalExecutionStackDeletions resize 0;
		};

		{
			_x params ["_args", "_function", "_delay", "_delta", "_totalDelta", "_handlerId", "_eventName", "_previousReturn", "_executionTime", "_executionCount"];

			if (_eventName in KH_var_temporalExecutionStackDeletions) then {
				continue;
			};

			if (_delay > 0) then {
				if (diag_tickTime >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, systemTime joinString ""];
						_totalDelta luaExecute "local totalDelta = ... return (sqf.systemTime() - totalDelta)";
					};

					_x set [7, _args call _function];
					_x set [3, _delta + _delay];
					_x set [9, _executionCount + 1];
				};
			}
			else {
				if (diag_frameNo >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, systemTime joinString ""];
						_totalDelta luaExecute "local totalDelta = ... return (sqf.systemTime() - totalDelta)";
					};

					_x set [7, _args call _function];
					_x set [3, _delta + (abs _delay)];
					_x set [9, _executionCount + 1];
				};
			};
		} forEach KH_var_temporalExecutionStack;
	}
];

[
	"KH_eve_temporalExecutionStackHandler", 
	{
		params ["_handlerId", "_deleteHandler", "_overrideTimeoutOnDeletion", "_conditionFailure"];
		private _currentHandler = KH_var_temporalExecutionStackMonitor get _handlerId;
		if (isNil "_currentHandler") exitWith {};
		_currentHandler params ["_timeoutArguments", "_timeoutFunction", "_handlerTickCounter", "_timeout", "_timeoutOnDeletion"];

		if _deleteHandler exitWith {
			if !(_handlerId in KH_var_temporalExecutionStackDeletions) then {
				KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handlerId;
		};

		if ((missionNamespace getVariable [_handlerTickCounter, 1]) >= _timeout) then {
			if !(_handlerId in KH_var_temporalExecutionStackDeletions) then {
				KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;
				_timeoutArguments call _timeoutFunction;
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handlerId;
		}
		else {
			missionNamespace setVariable [_handlerTickCounter, (missionNamespace getVariable [_handlerTickCounter, 1]) + 1];
		};
	}
] call CBA_fnc_addEventHandler;

[
	"KH_eve_registerCallback", 
	{
		params ["_arguments", ["_function", "", [""]], ["_caller", 2, [0]], ["_unscheduled", true, [true]], ["_callbackId", "", [""]]];
		[_callbackId, [_arguments, _function, _caller, _unscheduled] call KH_fnc_callSerializedFunction, _caller] call CBA_fnc_ownerEvent;		
	}
] call CBA_fnc_addEventHandler;

[
	[],
	{
		KH_var_networkingSettings = [
			parseNumber KH_var_networkingPort,
			parseNumber KH_var_networkingMaximumMessageSize,
			parseNumber KH_var_networkingReceiveBufferSize,
			parseNumber KH_var_networkingSendBufferSize,
			parseNumber KH_var_networkingConnectionTimeout,
			parseNumber KH_var_networkingSendTimeout,
			parseNumber KH_var_networkingReceiveTimeout,
			parseNumber KH_var_networkingClientStallTimeout,
			parseNumber KH_var_networkingKeepAliveTime,
			parseNumber KH_var_networkingKeepAliveInterval,
			parseNumber KH_var_networkingSendBatchSize,
			KH_var_networkingCompression
		];

		networkInitialize;
	},
	true,
	{
		(
			!(isNil "KH_var_serverAddress") &&
			!(isNil "KH_var_networkingPort") &&
			!(isNil "KH_var_networkingMaximumMessageSize") &&
			!(isNil "KH_var_networkingReceiveBufferSize") &&
			!(isNil "KH_var_networkingSendBufferSize") &&
			!(isNil "KH_var_networkingConnectionTimeout") &&
			!(isNil "KH_var_networkingSendTimeout") &&
			!(isNil "KH_var_networkingReceiveTimeout") &&
			!(isNil "KH_var_networkingClientStallTimeout") &&
			!(isNil "KH_var_networkingKeepAliveTime") &&
			!(isNil "KH_var_networkingKeepAliveInterval") &&
			!(isNil "KH_var_networkingSendBatchSize") &&
			!(isNil "KH_var_networkingCompression")
		);
	},
	false
] call KH_fnc_execute;

if isServer then {
	KH_var_serverGameSessionId = KH_var_gameSessionId;
	publicVariable "KH_var_serverGameSessionId";
	KH_var_serverMissionSessionId = KH_var_missionSessionId;
	publicVariable "KH_var_serverMissionSessionId";
	uiNamespace setVariable ["KH_var_serverMissionSessionId", KH_var_serverMissionSessionId];
	KH_var_missionStarted = false;
	publicVariable "KH_var_missionStarted";
	KH_var_playersLoaded = false;
	publicVariable "KH_var_playersLoaded";
	KH_var_disconnectedPlayerUids = [];
	publicVariable "KH_var_disconnectedPlayerUids";
	KH_var_adminMachine = clientOwner;
	publicVariable "KH_var_adminMachine";
	KH_var_allMachines = [clientOwner];
	publicVariable "KH_var_allMachines";
	KH_var_allIdMachines = createHashMap;
	publicVariable "KH_var_allIdMachines";
	KH_var_allCuratorMachines = [];
	publicVariable "KH_var_allCuratorMachines";
	KH_var_allHeadlessMachines = [];
	publicVariable "KH_var_allHeadlessMachines";
	KH_var_allHeadlessIdMachines = createHashMap;
	publicVariable "KH_var_allHeadlessIdMachines";
	KH_var_allHeadlessUnits = [];
	publicVariable "KH_var_allHeadlessUnits";
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
	KH_var_allPlayerControlledUnits = [];
	publicVariable "KH_var_allPlayerControlledUnits";
	KH_var_allPlayerControlledUnitsMachines = createHashMap;
	publicVariable "KH_var_allPlayerControlledUnitsMachines";
	KH_var_logicGroup = createGroup [sideLogic, false];
	publicVariable "KH_var_logicGroup";
	KH_var_helperLogic = KH_var_logicGroup createUnit ["Logic", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	publicVariable "KH_var_helperLogic";
	KH_var_helperObject = createVehicle ["KH_HelperSquare", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	publicVariable "KH_var_helperObject";
	KH_var_simpleHelperObject = createSimpleObject ["KH_HelperSquare", [0, 0, 0], false];
	publicVariable "KH_var_simpleHelperObject";
	KH_var_diagnosticsState = false;
	publicVariable "KH_var_diagnosticsState";
	KH_var_missionSuspended = false;
	publicVariable "KH_var_missionSuspended";
	KH_var_jipHandlers = createHashMap;
	KH_var_allEntities = entities [[], ["Animal"], true, false];
	KH_var_allLivingEntities = KH_var_allEntities select {alive _x;};
	KH_var_allDeadEntities = KH_var_allEntities select {!(alive _x);};
	KH_var_headlessClientTransfers = [];
	KH_var_entityArrayBuilderArrays = [];
	KH_var_groupArrayBuilderArrays = [];
	KH_var_initialSideRelations = [];
	KH_fnc_serverMissionLoadInit = {};
	KH_fnc_serverMissionStartInit = {};
	KH_fnc_serverPlayersLoadedInit = {};
	KH_fnc_serverMissionEndInit = {};

	{
		if ((_x getUserInfo 8) isNotEqualTo 0) then {
			KH_var_adminMachine = _x getUserInfo 1;
			publicVariable "KH_var_adminMachine";
			break;
		};
	} forEach allUsers;

	[
		"KH_eve_jipSetup",
		{
			params ["_name", "_arguments", "_dependency", "_unitRequired", "_jipId"];
			private _currentHandler = KH_var_jipHandlers get _jipId;
			private _continue = true;

			if !(isNil "_currentHandler") then {
				if ((_currentHandler select 4) isEqualTo _unitRequired) then {
					KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, []]];
					_continue = false;
				}
				else {
					{
						[_x] call KH_fnc_removeHandler;
					} forEach (_currentHandler select 5);
				};
			};

			if !_continue exitWith {};
			private _jipHandlers = [];
			KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, _jipHandlers]];
			
			private _joinType = if _unitRequired then {
				["KH_eve_playerLoaded", "KH_eve_headlessLoaded"];
			}
			else {
				["KH_eve_playerMissionLoaded", "KH_eve_headlessMissionLoaded"];
			};

			{
				_jipHandlers pushBack ([
					"CBA",
					_x,
					[_jipId, _x],
					{
						params ["_joiningMachine", "_id"];
						_args params ["_jipId", "_joinType"];

						if !(missionNamespace getVariable _jipId) exitWith {
							KH_var_jipHandlers deleteAt _jipId;
							[_handlerId] call KH_fnc_removeHandler;
						};

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
									if (_dependency isNotEqualTo _id) then {
										if (":" in _dependency) then {
											if ((isNull (objectFromNetId _dependency)) && (isNull (groupFromNetId _dependency))) then {
												_condition = false;
											};
										}
										else {
											if (isNil {KH_var_allIdMachines get _x;}) then {
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
												if (_x isNotEqualTo _id) then {
													if (":" in _x) then {
														if ((isNull (objectFromNetId _x)) && (isNull (groupFromNetId _x))) then {
															_condition = false;
															break;
														};
													}
													else {
														if (isNil {KH_var_allIdMachines get _x;}) then {
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
				] call KH_fnc_addEventHandler);
			} forEach _joinType;
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_persistentExecutionSetup",
		{
			params ["_arguments", "_function", "_target", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled", "_persistentExecutionId"];
			private _persistentEventId = ["KH_var_persistentEventId", _persistentExecutionId] joinString "_";

			if (_target isNil _persistentEventId) then {
				[
					"KH_eve_execution",
					[
						[
							["ENTITY", _target, "LOCAL"],
							"Local",
							[_persistentExecutionId, _persistentEventId],
							{
								params ["_entity", "_local"];
								_args params ["_persistentExecutionId", "_persistentEventId"];

								if (_entity getVariable _persistentExecutionId) then {
									(_entity getVariable _persistentEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled"];
									
									if _local then {
										[_arguments, _function, _caller, _unscheduled] call KH_fnc_callSerializedFunction;
									}
									else {
										[_sendoffArguments, _sendoffFunction, _caller, _unscheduled] call KH_fnc_callSerializedFunction;
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

			_target setVariable [_persistentEventId, [_arguments, _function, _sendoffArguments, _sendoffFunction, _caller, _unscheduled], true];
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerPresenceExecutionSetup",
		{
			params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId", "_units", "_jip"];

			[
				[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _nearId, _units],
				{
					params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId", "_units"];
				
					if ((isNull _object) || (_units isEqualTo []) || !(missionNamespace getVariable _nearId)) exitWith {
						[_handlerId] call KH_fnc_removeHandler;
					};

					private _deletions = [];

					{
						private _object = param [4];
						private _distance = param [6];

						if _present then {
							if ((_x distance _object) <= _distance) then {
								["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
								_deletions pushBack _forEachIndex;
							};
						}
						else {
							if ((_x distance _object) > _distance) then {
								["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
								_deletions pushBack _forEachIndex;
							};
						};
					} forEach _units;

					_units deleteAt _deletions;
				},
				true,
				0,
				false
			] call KH_fnc_execute;

			if _jip then {
				[
					"CBA",
					"KH_eve_playerLoaded",
					[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _nearId],
					{
						private _unit = param [3];
						_args params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId"];

						if !(missionNamespace getVariable _nearId) exitWith {
							[_handlerId] call KH_fnc_removeHandler;
						};

						[
							[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _unit],
							{
								params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_unit"];
								_this set [7, _unit getVariable ["KH_var_playerUnit", _unit]];

								if !(missionNamespace getVariable _nearId) exitWith {
									[_handlerId] call KH_fnc_removeHandler;
								};
								
								if _present then {
									if ((_unit distance _object) <= _distance) then {
										["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
										[_handlerId] call KH_fnc_removeHandler;
									};
								}
								else {
									if ((_unit distance _object) > _distance) then {
										["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
										[_handlerId] call KH_fnc_removeHandler;
									};
								};
							},
							true,
							0,
							false
						] call KH_fnc_execute;
					}
				] call KH_fnc_addEventHandler;
			};
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerMissionPreloaded", 
		{
			params ["_machineId"];
			KH_var_allMachines pushBackUnique _machineId;
			publicVariable "KH_var_allMachines";
			KH_var_allPlayerMachines pushBackUnique _machineId;
			publicVariable "KH_var_allPlayerMachines";
			private "_uid";
			private "_id";
			
			{
				if ((_x getUserInfo 1) isEqualTo _machineId) then {
					_uid = _x getUserInfo 2;
					_id = _x getUserInfo 0;
					KH_var_allPlayerUidMachines set [_uid, _machineId];
					publicVariable "KH_var_allPlayerUidMachines";
					KH_var_allPlayerIdMachines set [_id, _machineId];
					publicVariable "KH_var_allPlayerIdMachines";
					KH_var_allIdMachines set [_id, _machineId];
					publicVariable "KH_var_allIdMachines";
					missionNamespace setVariable ["KH_var_steamId", _uid, _machineId];
					missionNamespace setVariable ["KH_var_directPlayId", _directPlayId, _machineId];
					break;
				};
			} forEach allUsers;
			
			if KH_var_playersLoaded then {
				KH_var_jipPlayerMachines pushBackUnique _machineId;
				publicVariable "KH_var_jipPlayerMachines";
			};

			if ((admin _machineId) isNotEqualTo 0) then {
				KH_var_adminMachine = _machineId;
				publicVariable "KH_var_adminMachine";
			};

			[
				[_uid],
				{
					params ["_uid"];

					if (profileNamespace isNil "KH_var_steamId") then {
						profileNamespace setVariable ["KH_var_steamId", _uid];
					};					
				},
				_machineId,
				true,
				false
			] call KH_fnc_execute;

			["KH_eve_playerMissionLoaded", [_machineId, _uid, _id]] call CBA_fnc_globalEvent;
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_headlessMissionPreloaded", 
		{
			params ["_machineId"];
			KH_var_allMachines pushBackUnique _machineId;
			publicVariable "KH_var_allMachines";
			KH_var_allHeadlessMachines pushBackUnique _machineId;
			publicVariable "KH_var_allHeadlessMachines";
			private _id = "";

			{
				if ((_x getUserInfo 1) isEqualTo _machineId) then {
					_id = _x getUserInfo 0;
					KH_var_allIdMachines set [_id, _machineId];
					publicVariable "KH_var_allIdMachines";
					KH_var_allHeadlessIdMachines set [_id, _machineId];
					publicVariable "KH_var_allHeadlessIdMachines";
					break;
				};
			} forEach allUsers;

			["KH_eve_headlessMissionLoaded", [_machineId, _id]] call CBA_fnc_globalEvent;
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerLoaded",
		{
			private _unit = param [3];

			if KH_var_playersLoaded then {
				KH_var_jipPlayerUnits pushBackUnique _unit;
				publicVariable "KH_var_jipPlayerUnits";
			};

			KH_var_allPlayerUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerUnits";
			private _arrayBuilderArray = _unit getVariable ["KH_var_assignedEntityArrayBuilderArrays", []];

			if (_arrayBuilderArray isNotEqualTo []) then {
				{
					private _entityArray = missionNamespace getVariable [_x, []];

					if !(_unit in _entityArray) then {
						_entityArray pushBackUnique _unit;
						missionNamespace setVariable [_x, _entityArray, true];
					};
				} forEach _arrayBuilderArray;
			};
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_headlessLoaded",
		{
			private _headlessClientOwner = param [0];
			private _headlessClient = param [2];
			KH_var_allHeadlessUnits pushBackUnique _headlessClient;
			publicVariable "KH_var_allHeadlessUnits";
			private _arrayBuilderArray = _headlessClient getVariable ["KH_var_assignedEntityArrayBuilderArrays", []];

			if (_arrayBuilderArray isNotEqualTo []) then {
				{
					private _entityArray = missionNamespace getVariable [_x, []];

					if !(_headlessClient in _entityArray) then {
						_entityArray pushBackUnique _headlessClient;
						missionNamespace setVariable [_x, _entityArray, true];
					};
				} forEach _arrayBuilderArray;
			};

			private _assignedEntities = [];
			private _assignedEntitiesRecreate = [];

			{
				_x params ["_unit", "_owner", "_recreate"];
				
				if ((vehicleVarName _headlessClient) isEqualTo _owner) then {
					if !_recreate then {
						_assignedEntities pushBack _unit;
					}
					else {
						_assignedEntitiesRecreate pushBack _unit;
					};
				};
			} forEach KH_var_headlessClientTransfers;
			
			_unit setVariable ["KH_var_headlessClientTransferInit", _unit getVariable ["KH_var_headlessClientTransferInit", {}], _headlessClientOwner];

			if (_assignedEntities isNotEqualTo []) then {
				[
					_assignedEntities, 
					_headlessClientOwner, 
					false, 
					{
						params ["_unit"];
						[_unit] call (_unit getVariable "KH_var_headlessClientTransferInit");
					}
				] call KH_fnc_setOwnership;
			};

			if (_assignedEntitiesRecreate isNotEqualTo []) then {
				[
					_assignedEntitiesRecreate, 
					_headlessClientOwner, 
					true, 
					{
						params ["_unit"];
						[_unit] call (_unit getVariable "KH_var_headlessClientTransferInit");
					}
				] call KH_fnc_setOwnership;
			};
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerSwitched", 
		{
			private _newUnit = param [3];
			private _previousUnit = param [4];

			{
				private _units = missionNamespace getVariable _x;

				if ([_units, [_previousUnit]] call KH_fnc_deleteArrayElements) then {
					_units pushBackUnique _newUnit;
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits"];
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerControlledUnitChanged", 
		{
			private _owner = param [0];
			private _unit = param [3];
			private _oldUnit = KH_var_allPlayerControlledUnitsMachines get _owner;

			if !(isNil "_oldUnit") then {
				KH_var_allPlayerControlledUnits deleteAt (KH_var_allPlayerControlledUnits find _oldUnit);
			};

			KH_var_allPlayerControlledUnitsMachines set [_owner, _unit];
			publicVariable "KH_var_allPlayerControlledUnitsMachines";
			KH_var_allPlayerControlledUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerControlledUnits";
		}
	] call CBA_fnc_addEventHandler;

	addMissionEventHandler [
		"OnUserAdminStateChanged", 
		{
			params ["_networkId", "_loggedIn", "_votedIn"];
			private _machineId = _networkId getUserInfo 1;
			KH_var_adminMachine = [2, _machineId] select (_loggedIn || _votedIn);
			publicVariable "KH_var_adminMachine";
			["KH_eve_adminChanged", [_machineId, _loggedIn, _votedIn]] call CBA_fnc_globalEvent;
		}
	];

	addMissionEventHandler [
		"EntityCreated", 
		{
			params ["_entity"];
			KH_var_allEntities pushBackUnique _entity;

			if (alive _entity) then {
				KH_var_allLivingEntities pushBackUnique _entity;
			};

			if (_entity isKindOf "CAManBase") then {
				[_entity] call KH_fnc_medicalSetup;
			};
		}
	];

	addMissionEventHandler [
		"EntityKilled", 
		{
			params ["_entity", "_killer", "_instigator"];
			KH_var_allDeadEntities pushBackUnique _entity;

			if (_entity in KH_var_allLivingEntities) then {
				KH_var_allLivingEntities deleteAt (KH_var_allLivingEntities find _entity);
			};
		}
	];

	addMissionEventHandler [
		"EntityRespawned",
		{
			params ["_newEntity", "_oldEntity"];
			
			{
				private _units = missionNamespace getVariable _x;

				if ([_units, [_oldEntity]] call KH_fnc_deleteArrayElements) then {
					_units pushBackUnique _newEntity;
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits"];

			{
				private _entityArray = missionNamespace getVariable [_x, []];

				if (_oldEntity in _entityArray) then {
					_entityArray deleteAt (_entityArray find _oldEntity);
					_entityArray pushBackUnique _newEntity;
					missionNamespace setVariable [_x, _entityArray, true];
				};
			} forEach KH_var_entityArrayBuilderArrays;

			if (_oldEntity in KH_var_allLivingEntities) then {
				_oldEntity deleteAt (KH_var_allLivingEntities find _oldEntity);
			};
		}
	];

	addMissionEventHandler [
		"EntityDeleted", 
		{
			params ["_entity"];
			
			{
				if ([missionNamespace getVariable _x, [_entity]] call KH_fnc_deleteArrayElements) then {
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits", "KH_var_allHeadlessUnits", "KH_var_allPlayerControlledUnits"];

			{
				[missionNamespace getVariable _x, [_entity]] call KH_fnc_deleteArrayElements;
			} forEach ["KH_var_allEntities", "KH_var_allLivingEntities", "KH_var_allDeadEntities"];
		}
	];

	addMissionEventHandler [
		"HandleDisconnect", 
		{
			params ["_unit", "_id", "_uid", "_name"];
			private _machineId = KH_var_allPlayerUidMachines get _uid;

			if !(isNil "_machineId") then {
				private _attributes = [];

				if !(isNull _unit) then {					
					_attributes = if (alive _unit) then {
						[_unit] call KH_fnc_getUnitAttributes;
					}
					else {
						[];
					};

					KH_var_disconnectedPlayerUids pushBackUnique _uid;
					publicVariable "KH_var_disconnectedPlayerUids";

					{
						if ([missionNamespace getVariable _x, [_unit]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach [
						"KH_var_allPlayerUnits", 
						"KH_var_jipPlayerUnits", 
						"KH_var_initialPlayerUnits",
						"KH_var_allPlayerControlledUnits"
					];
				};

				{
					if ([missionNamespace getVariable _x, [_machineId]] call KH_fnc_deleteArrayElements) then {
						publicVariable _x;
					};
				} forEach [
					"KH_var_allMachines", 
					"KH_var_allCuratorMachines", 
					"KH_var_allPlayerMachines", 
					"KH_var_jipPlayerMachines",
					"KH_var_initialPlayerMachines"
				];

				KH_var_allIdMachines deleteAt _id;
				publicVariable "KH_var_allIdMachines";
				KH_var_allPlayerUidMachines deleteAt _uid;
				publicVariable "KH_var_allPlayerUidMachines";
				KH_var_allPlayerIdMachines deleteAt _id;
				publicVariable "KH_var_allPlayerIdMachines";
				["KH_eve_playerDisconnected", [_machineId, _uid, _id, _unit, _attributes]] call CBA_fnc_globalEvent;
			}
			else {
				private _machineId = KH_var_allHeadlessIdMachines get _id;

				if !(isNil "_machineId") then {
					{
						if ([missionNamespace getVariable _x, [_unit]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach ["KH_var_allHeadlessUnits"];

					{
						if ([missionNamespace getVariable _x, [_machineId]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach [
						"KH_var_allMachines", 
						"KH_var_allHeadlessMachines"
					];

					KH_var_allIdMachines deleteAt _id;
					publicVariable "KH_var_allIdMachines";
					KH_var_allHeadlessIdMachines deleteAt _id;
					publicVariable "KH_var_allHeadlessIdMachines";
				};

				["KH_eve_headlessClientDisconnected", [_machineId, _id]] call CBA_fnc_globalEvent;
			};
		}
	];

	[
		[],
		{
			if KH_var_diagnosticsState then {
				missionNamespace setVariable ["KH_var_diagnosticsFramerateServer", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
				missionNamespace setVariable ["KH_var_diagnosticsLocalUnitsServer", {local _x;} count allUnits, KH_var_adminMachine];
			};
		},
		true,
		1,
		false
	] call KH_fnc_execute;

	[
		[],
		{
			if (KH_var_adminMachine isNotEqualTo clientOwner) then {
				{
					_x setVariable ["KH_var_playerViewDistance", _x getVariable "KH_var_playerViewDistance", KH_var_adminMachine];
					_x setVariable ["KH_var_playerAspectRatio", _x getVariable "KH_var_playerAspectRatio", KH_var_adminMachine];
					_x setVariable ["KH_var_playerCameraPosition", _x getVariable "KH_var_playerCameraPosition", KH_var_adminMachine];
					_x setVariable ["KH_var_playerCameraDirection", _x getVariable "KH_var_playerCameraDirection", KH_var_adminMachine];
					_x setVariable ["KH_var_playerUnit", _x getVariable "KH_var_playerUnit", KH_var_adminMachine];
				} forEach KH_var_allPlayerUnits;
			};
		},
		true,
		0,
		false
	] call KH_fnc_execute;

	{
		[_x] call KH_fnc_medicalSetup;
	} forEach (KH_var_allEntities select {_x isKindOf "CAManBase";});
};

if hasInterface then {
	KH_fnc_playerMissionLoadInit = {};
	KH_fnc_playerMissionStartInit = {};
	KH_fnc_playerLoadInit = {};
	KH_fnc_playerPlayersLoadedInit = {};
	KH_fnc_playerKilledInit = {};
	KH_fnc_playerRespawnInit = {};
	KH_fnc_playerSwitchInit = {};
	KH_fnc_playerControlledUnitChangeInit = {};
	KH_fnc_playerMissionEndInit = {};

	[
		"unit", 
		{
			params ["_unit"];
			private _previousUnit = KH_var_playerUnit;
			KH_var_playerUnit = _unit;
			player setVariable ["KH_var_playerUnit", KH_var_playerUnit, true];
			["KH_eve_playerControlledUnitChanged", [clientOwner, getPlayerUID player, getPlayerID player, _unit, _previousUnit, [_unit, true] call KH_fnc_getEntityVariableName]] call CBA_fnc_globalEvent;
			[_previousUnit, _unit] call KH_fnc_playerControlledUnitChangeInit;
		},
		true
	] call CBA_fnc_addPlayerEventHandler;

	addMissionEventHandler [
		"TeamSwitch", 
		{
			params ["_previousUnit", "_newUnit"];
			[_previousUnit] call KH_fnc_playerSwitchInit;
			["KH_eve_playerSwitched", [clientOwner, getPlayerUID _newUnit, getPlayerID _newUnit, _newUnit, _previousUnit]] call CBA_fnc_globalEvent;
			
			if (KH_var_playerRespawnedEventHandler isNotEqualTo []) then {
				[KH_var_playerRespawnedEventHandler] call KH_fnc_removeHandler;
			};

			KH_var_playerRespawnedEventHandler = [
				["ENTITY", _newUnit, "LOCAL"],
				"Respawn",
				[],
				{
					params ["_unit", "_corpse"];
					_corpse setVariable ["KH_var_playerUnit", _unit];
					_corpse setVehicleVarName "";

					[
						[_corpse],
						{
							params ["_corpse"];
							_corpse setVehicleVarName "";
						},
						"GLOBAL",
						true,
						true
					] call KH_fnc_execute;
					
					[_corpse] call KH_fnc_playerRespawnInit;
					["KH_eve_playerRespawned", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _corpse]] call CBA_fnc_globalEvent;
					nil;
				}
			] call KH_fnc_addEventHandler;

			if (KH_var_playerKilledEventHandler isNotEqualTo []) then {
				[KH_var_playerKilledEventHandler] call KH_fnc_removeHandler;
			};

			KH_var_playerKilledEventHandler = [
				["ENTITY", _newUnit, "LOCAL"],
				"Killed",
				[],
				{
					params ["_unit", "_killer", "_instigator"];
					[_killer, _instigator] call KH_fnc_playerKilledInit;
					["KH_eve_playerKilled", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _killer, _instigator]] call CBA_fnc_globalEvent;
				}
			] call KH_fnc_addEventHandler;
		}
	];

	addMissionEventHandler [
		"Draw2D", 
		{
			if (KH_var_drawUi2dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi2dExecutionStack append KH_var_drawUi2dExecutionStackAdditions;
				KH_var_drawUi2dExecutionStackAdditions resize 0;
			};

			if (KH_var_drawUi2dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi2dExecutionStackDeletions) then {
						KH_var_drawUi2dExecutionStackDeletions set [KH_var_drawUi2dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi2dExecutionStack;

				KH_var_drawUi2dExecutionStack deleteAt (KH_var_drawUi2dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi2dExecutionStackDeletions resize 0;
			};

			{
				_x params ["_args", "_function", "_handlerId", "_eventName", "_previousReturn", "_executionTime"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi2dExecutionStack;
		}
	];

	addMissionEventHandler [
		"Draw3D", 
		{
			if (KH_var_drawUi3dOrphanExecutionStack isNotEqualTo []) then {
				{
					_x params ["_type", "_arguments"];

					switch _type do {
						case "LINE": {
							drawLine3D _arguments;
						};

						case "LASER": {
							drawLaser _arguments;
						};

						case "ICON": {
							drawIcon3D _arguments;
						};
					};
				} forEach KH_var_drawUi3dOrphanExecutionStack;

				KH_var_drawUi3dOrphanExecutionStack resize 0;
			};

			if (KH_var_drawUi3dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi3dExecutionStack append KH_var_drawUi3dExecutionStackAdditions;
				KH_var_drawUi3dExecutionStackAdditions resize 0;
			};

			if (KH_var_drawUi3dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi3dExecutionStackDeletions) then {
						KH_var_drawUi3dExecutionStackDeletions set [KH_var_drawUi3dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi3dExecutionStack;

				KH_var_drawUi3dExecutionStack deleteAt (KH_var_drawUi3dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi3dExecutionStackDeletions resize 0;
			};

			{
				_x params ["_args", "_function", "_handlerId", "_eventName", "_previousReturn", "_executionTime"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi3dExecutionStack;
		}
	];

	[
		"KH_eve_drawUiExecutionStackHandler", 
		{
			params ["_handlerId", "_overrideTimeoutOnDeletion", "_conditionFailure"];
			private _currentHandler = KH_var_drawUiExecutionStackMonitor get _handlerId;
			if (isNil "_currentHandler") exitWith {};
			_currentHandler params ["_event", "_timeoutArguments", "_timeoutFunction", "_timeoutOnDeletion"];
			private _drawType = [KH_var_drawUi2dExecutionStackAdditions, KH_var_drawUi3dExecutionStackAdditions] select _event;

			if !(_handlerId in _deletionType) then {
				_deletionType insert [0, [_handlerId]];

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			KH_var_drawUiExecutionStackMonitor deleteAt _handlerId;
		}
	] call CBA_fnc_addEventHandler;

	[
		[],
		{
			if KH_var_diagnosticsState then {
				player setVariable ["KH_var_diagnosticsFramerate", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
			};
		},
		true,
		1,
		false
	] call KH_fnc_execute;

	[
		[],
		{
			player setVariable ["KH_var_playerViewDistance", viewDistance, 2];
			player setVariable ["KH_var_playerAspectRatio", getResolution select 4, 2];
			player setVariable ["KH_var_playerCameraPosition", positionCameraToWorld [0, 0, 0], 2];
			player setVariable ["KH_var_playerCameraDirection", getCameraViewDirection player, 2];
		},
		true,
		0,
		false
	] call KH_fnc_execute;
};

if (!isServer && !hasInterface) then {
	KH_fnc_headlessMissionLoadInit = {};
	KH_fnc_headlessMissionStartInit = {};
	KH_fnc_headlessLoadInit = {};
	KH_fnc_headlessPlayersLoadedInit = {};
	KH_fnc_headlessMissionEndInit = {};

	[
		[],
		{
			if KH_var_diagnosticsState then {
				player setVariable ["KH_var_diagnosticsFramerate", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
				player setVariable ["KH_var_diagnosticsLocalUnits", {local _x;} count allUnits, KH_var_adminMachine];
			};
		},
		true,
		1,
		false
	] call KH_fnc_execute;
};

{
	luaExecute _x;
} forEach KH_var_preInitLuaExecutions;

nil;