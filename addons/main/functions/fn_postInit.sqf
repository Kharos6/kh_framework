isNil {
	KH_var_missionLoaded = true;

	if isServer then {
		switch KH_var_incapacitationAvailability do {
			case 0: {
				KH_var_allowIncapacitation = false;
				KH_var_allowAiIncapacitation = false;
			};

			case 1: {
				KH_var_allowIncapacitation = true;
				KH_var_allowAiIncapacitation = false;
			};

			case 2: {
				KH_var_allowIncapacitation = true;
				KH_var_allowAiIncapacitation = true;
			};

			default {
				KH_var_allowIncapacitation = true;
				KH_var_allowAiIncapacitation = false;
			};
		};

		publicVariable "KH_var_allowIncapacitation";
		publicVariable "KH_var_allowAiIncapacitation";

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

		if isMultiplayer then {
			[[], {systemChat "KH FRAMEWORK - MISSION LOADED"; diag_log "KH FRAMEWORK - MISSION LOADED";}, ["SERVER", "ADMIN"] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;
		}
		else {
			diag_log "KH FRAMEWORK - MISSION LOADED";
		};

		[
			[],
			{
				KH_var_missionStarted = true;
				publicVariable "KH_var_missionStarted";
				call KH_fnc_serverMissionStartInit;
				[[], "KH_fnc_playerMissionStartInit", "PLAYERS", true, false] call KH_fnc_execute;
				[[], "KH_fnc_headlessMissionStartInit", "HEADLESS", true, false] call KH_fnc_execute;

				if isMultiplayer then {
					[[], {systemChat "KH FRAMEWORK - MISSION STARTED"; diag_log "KH FRAMEWORK - MISSION STARTED";}, ["SERVER", "ADMIN"] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;
				}
				else {
					diag_log "KH FRAMEWORK - MISSION STARTED";
				};

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

						if isMultiplayer then {
							[[], {systemChat "KH FRAMEWORK - PLAYERS LOADED"; diag_log "KH FRAMEWORK - PLAYERS LOADED";}, ["SERVER", "ADMIN"] + KH_var_allCuratorMachines, true, false] call KH_fnc_execute;
						}
						else {
							diag_log "KH FRAMEWORK - PLAYERS LOADED";
						};
					},
					true,
					{
						private _initialPlayerCount = 0;

						{
							if (!(_x getUserInfo 7) && ((_x getUserInfo 6) > 8)) then {
								_initialPlayerCount = _initialPlayerCount + 1;
							};
						} forEach allUsers;
						
						((((count KH_var_allPlayerMachines) >= _initialPlayerCount)) && ((count KH_var_allPlayerUnits) >= _initialPlayerCount)) || (CBA_missionTime > 60);
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

		[
			[],
			{
				KH_var_networkingSettings = [
					KH_var_networkingPort,
					KH_var_networkingMaximumMessageSize,
					KH_var_networkingReceiveBufferSize,
					KH_var_networkingSendBufferSize,
					KH_var_networkingConnectionTimeout,
					KH_var_networkingSendTimeout,
					KH_var_networkingReceiveTimeout,
					KH_var_networkingClientStallTimeout,
					KH_var_networkingKeepAliveTime,
					KH_var_networkingKeepAliveInterval,
					KH_var_networkingSendBatchSize,
					KH_var_networkingCompression,
					KH_var_networkingCoalesceMessages,
					KH_var_networkingMaximumCoalesceSize,
					KH_var_networkingMaximumCoalescedMessages,
					KH_var_networkingCoalesceDelay
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
					!(isNil "KH_var_networkingCompression") &&
					!(isNil "KH_var_networkingCoalesceMessages") &&
					!(isNil "KH_var_networkingMaximumCoalesceSize") &&
					!(isNil "KH_var_networkingMaximumCoalescedMessages") &&
					!(isNil "KH_var_networkingCoalesceDelay")
				);
			},
			false
		] call KH_fnc_execute;
	};
	
	if hasInterface then {
		"KH_var_khDisplayLayer" cutRsc ["KH_ResourceKHDisplay", "PLAIN", -1, true, true];
		call KH_fnc_playerMissionLoadInit;
		["KH_eve_playerMissionPreloaded", [clientOwner]] call CBA_fnc_serverEvent;

		[
			[],
			{
				call KH_fnc_playerLoadInit;
				["KH_eve_playerLoaded", [clientOwner, getPlayerUID player, getPlayerID player, player, [player, true] call KH_fnc_getEntityVariableName]] call CBA_fnc_globalEvent;				

				if (KH_var_playerRespawnedEventHandler isNotEqualTo []) then {
					[KH_var_playerRespawnedEventHandler] call KH_fnc_removeHandler;
				};

				KH_var_playerRespawnedEventHandler = [
					["ENTITY", player, "LOCAL"],
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
					["ENTITY", player, "LOCAL"],
					"Killed",
					[],
					{
						params ["_unit", "_killer", "_instigator"];
						[_killer, _instigator] call KH_fnc_playerKilledInit;
						["KH_eve_playerKilled", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _killer, _instigator]] call CBA_fnc_globalEvent;
					}
				] call KH_fnc_addEventHandler;

				{
					luaExecute _x;
				} forEach KH_var_loadInitLuaExecutions;
			},
			true,
			{(!(isNull player) && (alive player));},
			false
		] call KH_fnc_execute;

		if KH_var_khMedical then {
			private _control = (uiNamespace getVariable ["KH_var_khDisplay", displayNull]) ctrlCreate ["RscStructuredText", -1];
			private _controlBackground = (uiNamespace getVariable ["KH_var_khDisplay", displayNull]) ctrlCreate ["RscText", -1];
			_control ctrlShow false;
			_controlBackground ctrlShow false;
			_controlBackground ctrlSetBackgroundColor [0, 0, 0, 0.45];
			_controlBackground ctrlCommit 0;

			[
				[_control, _controlBackground],
				{
					params ["_control", "_controlBackground"];

					if ((KH_var_healthDisplay isNotEqualTo "NONE") && !visibleMap) then {
						if (KH_var_healthDisplayHideFullHealth && ((damage KH_var_playerUnit) isEqualTo 0)) then {
							_control ctrlShow false;
							_controlBackground ctrlShow false;
						}
						else {
							_control ctrlShow true;
							_controlBackground ctrlShow true;

							if (KH_var_healthDisplay isEqualTo "BAR") then {
								_control ctrlSetPosition (
									[
										[KH_var_healthDisplayBarPositionX, KH_var_healthDisplayBarPositionY, KH_var_healthDisplayBarSizeX * (1 - (damage KH_var_playerUnit)), KH_var_healthDisplayBarSizeY], 
										false
									] call KH_fnc_parseNormalizedScreenTransforms
								);
								
								_controlBackground ctrlSetPosition (
									[
										[KH_var_healthDisplayBarPositionX, KH_var_healthDisplayBarPositionY, KH_var_healthDisplayBarSizeX, KH_var_healthDisplayBarSizeY], 
										false
									] call KH_fnc_parseNormalizedScreenTransforms
								);

								_control ctrlSetAngle [KH_var_healthDisplayBarAngle, 0, 0, true];
								_controlBackground ctrlSetAngle [KH_var_healthDisplayBarAngle, 0, 0, true];
								_control ctrlSetStructuredText (parseText "");
								_control ctrlSetBackgroundColor [profileNamespace getVariable ["GUI_BCG_RGB_R", 0.13], profileNamespace getVariable ["GUI_BCG_RGB_G", 0.54], profileNamespace getVariable ["GUI_BCG_RGB_B", 0.21], 0.9];
							}
							else {
								private _percentageColor = [
									profileNamespace getVariable ["GUI_BCG_RGB_R", 0.13], 
									profileNamespace getVariable ["GUI_BCG_RGB_G", 0.54], 
									profileNamespace getVariable ["GUI_BCG_RGB_B", 0.21], 
									1
								] call BIS_fnc_colorRGBAtoHTML;

								private _position = [
									[KH_var_healthDisplayPercentagePositionX, KH_var_healthDisplayPercentagePositionY, KH_var_healthDisplayPercentageSizeX, KH_var_healthDisplayPercentageSizeY], 
									false
								] call KH_fnc_parseNormalizedScreenTransforms;

								_control ctrlSetPosition _position;
								_controlBackground ctrlSetPosition _position;
								_control ctrlSetAngle [KH_var_healthDisplayPercentageAngle, 0, 0, true];
								_controlBackground ctrlSetAngle [KH_var_healthDisplayPercentageAngle, 0, 0, true];
								_control ctrlSetStructuredText (parseText (["<t align='center' valign='middle' size='", KH_var_healthDisplayPercentageTextSize, "' color='", _percentageColor, "' font='EtelkaMonospaceProBold'>", ((1 - (damage KH_var_playerUnit)) * 100) toFixed 0, "%</t>"] joinString ""));
								_control ctrlSetBackgroundColor [0, 0, 0, 0.45];
							};

							_control ctrlCommit 0;
							_controlBackground ctrlCommit 0;
						};
					}
					else {
						_control ctrlShow false;
						_controlBackground ctrlShow false;
					};
				},
				true,
				0,
				false
			] call KH_fnc_execute;
		};
	};

	if (!isServer && !hasInterface) then {
		call KH_fnc_headlessMissionLoadInit;
		["KH_eve_headlessMissionPreloaded", [clientOwner]] call CBA_fnc_serverEvent;

		[
			[],
			{
				call KH_fnc_headlessLoadInit;
				["KH_eve_headlessLoaded", [clientOwner, getPlayerID player, player, [player, true] call KH_fnc_getEntityVariableName]] call CBA_fnc_globalEvent;				

				{
					luaExecute _x;
				} forEach KH_var_loadInitLuaExecutions;
			},
			true,
			{(!(isNull player) && (alive player));},
			false
		] call KH_fnc_execute;
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

	if isServer then {
		if !(isNil "KH_var_missionStartSuspension") then {
			{
				KH_var_missionStartSuspensionEntities = [];
				private _timeMultiplier = timeMultiplier;
				setTimeMultiplier 0.1;

				if (KH_var_missionStartSuspension isEqualTo 2) then {
					{
						if (simulationEnabled _x) then {
							_x enableSimulationGlobal false;
							(objectParent _x) enableSimulationGlobal false;
							KH_var_missionStartSuspensionEntities insert [-1, [_x, objectParent _x], true];
						};
					} forEach (allUnits select {(owner _x) isNotEqualTo KH_var_adminMachine;});
				}
				else {
					{
						_x enableSimulationGlobal false;
						(objectParent _x) enableSimulationGlobal false;
						KH_var_missionStartSuspensionEntities insert [-1, [_x, objectParent _x], true];
					} forEach (KH_var_allPlayerUnits select {(owner _x) isNotEqualTo KH_var_adminMachine;});
				};

				[
					"KH_eve_playerLoaded",
					{
						private _unit = param [3];
						_x enableSimulationGlobal false;
						(objectParent _x) enableSimulationGlobal false;
						KH_var_missionStartSuspensionEntities pushBackUnique _unit;
					}
				] call CBA_fnc_addEventHandler;

				[
					[],
					{
						with uiNamespace do {
							if (isNil "KH_var_suspensionDisplay") then {
								KH_var_suspensionDisplay = ["RscText", "PLAYERS LOADING...", [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
							}
							else {
								if (isNull KH_var_suspensionDisplay) then {
									KH_var_suspensionDisplay = ["RscText", "PLAYERS LOADING...", [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
								};
							};
						};
					},
					KH_var_allPlayerMachines - [KH_var_adminMachine],
					true,
					["JIP", {!KH_var_playersLoaded;}, true, ""]
				] call KH_fnc_execute;

				[
					[_timeMultiplier],
					{
						params ["_timeMultiplier"];

						{
							_x enableSimulationGlobal true;
						} forEach KH_var_missionStartSuspensionEntities;

						setTimeMultiplier _timeMultiplier;

						[
							[],
							{
								with uiNamespace do {
									if !(isNil "KH_var_suspensionDisplay") then {
										ctrlDelete KH_var_suspensionDisplay;
										KH_var_suspensionDisplay = nil;
									};
								};
							},
							"PLAYERS",
							true,
							false
						] call KH_fnc_execute;
					},
					true,
					{KH_var_playersLoaded;},
					false
				] call KH_fnc_execute;
			} forEach allUnits;
		};

		{
			[_x] call KH_fnc_medicalSetup;
		} forEach (KH_var_allEntities select {_x isKindOf "CAManBase";});
	};
};

nil;