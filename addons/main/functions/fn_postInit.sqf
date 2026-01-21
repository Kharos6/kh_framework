isNil {
	if KH_var_isMenu exitWith {};

	[
		[],
		{
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

				if !(isNil "KH_var_missionStartSuspension") then {
					KH_var_missionSuspended = true;
					publicVariable "KH_var_missionSuspended";
				};

				{
					call _x;
				} forEach KH_var_serverMissionLoadStack;

				call KH_fnc_serverMissionLoadInit;

				if isMultiplayer then {
					[[], {systemChat "KH FRAMEWORK - MISSION LOADED"; diag_log "KH FRAMEWORK - MISSION LOADED";}, ["SERVER", "ADMIN"], true, false] call KH_fnc_execute;
				}
				else {
					diag_log "KH FRAMEWORK - MISSION LOADED";
				};

				[
					[],
					{
						KH_var_missionStarted = true;
						publicVariable "KH_var_missionStarted";

						{
							call _x;
						} forEach KH_var_serverMissionStartStack;

						call KH_fnc_serverMissionStartInit;

						[
							[],
							{
								{
									call _x;
								} forEach KH_var_playerMissionStartStack;

								call KH_fnc_playerMissionStartInit;
							},
							"PLAYERS",
							true,
							false
						] call KH_fnc_execute;

						[
							[],
							{
								{
									call _x;
								} forEach KH_var_headlessMissionStartStack;

								call KH_fnc_headlessMissionStartInit;
							},
							"HEADLESS",
							true,
							false
						] call KH_fnc_execute;

						if isMultiplayer then {
							[[], {systemChat "KH FRAMEWORK - MISSION STARTED"; diag_log "KH FRAMEWORK - MISSION STARTED";}, ["SERVER", "ADMIN"], true, false] call KH_fnc_execute;
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

								{
									call _x;
								} forEach KH_var_serverPlayersLoadedStack;

								call KH_fnc_serverPlayersLoadedInit;

								[
									[],
									{
										{
											call _x;
										} forEach KH_var_playerPlayersLoadedStack;

										call KH_fnc_playerPlayersLoadedInit;
									},
									"PLAYERS",
									true,
									false
								] call KH_fnc_execute;

								[
									[],
									{
										{
											call _x;
										} forEach KH_var_headlessPlayersLoadedStack;

										call KH_fnc_headlessPlayersLoadedInit;
									},
									"HEADLESS",
									true,
									false
								] call KH_fnc_execute;

								["KH_eve_playersLoaded", [], "GLOBAL", false] call KH_fnc_triggerCbaEvent;

								if isMultiplayer then {
									[[], {systemChat "KH FRAMEWORK - PLAYERS LOADED"; diag_log "KH FRAMEWORK - PLAYERS LOADED";}, ["SERVER", "ADMIN"], true, false] call KH_fnc_execute;
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

						if !(isNil "KH_var_missionStartSuspension") then {
							{
								KH_var_missionSuspensionEntities = [];
								private _timeMultiplier = timeMultiplier;
								setTimeMultiplier 0.1;

								{
									if !(_x in KH_var_missionSuspensionEntities) then {
										_x setVariable ["KH_var_originalSimulationState", simulationEnabled _x];
										_x enableSimulationGlobal false;
									};

									private _parent = objectParent _x;
									
									if !(_parent in KH_var_missionSuspensionEntities) then {
										_parent setVariable ["KH_var_originalSimulationState", simulationEnabled _parent];
										_parent enableSimulationGlobal false;
									};
									
									KH_var_missionSuspensionEntities insert [-1, [_x, _parent], true];
								} forEach ([KH_var_allPlayerUnits, allUnits] select (KH_var_missionStartSuspension isEqualTo 2));

								private _playerLoadHandler = [
									"CBA",
									"KH_eve_playerLoaded",
									[],
									{
										private _unit = param [3];

										if !(_unit in KH_var_missionSuspensionEntities) then {
											_unit setVariable ["KH_var_originalSimulationState", simulationEnabled _unit];
											_unit enableSimulationGlobal false;
										};

										private _parent = objectParent _unit;

										if !(_parent in KH_var_missionSuspensionEntities) then {
											_parent setVariable ["KH_var_originalSimulationState", simulationEnabled _parent];
											_parent enableSimulationGlobal false;
										};
									
										KH_var_missionSuspensionEntities insert [-1, [_unit, _parent], true];
									}
								] call KH_fnc_addEventHandler;

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
									[_timeMultiplier, _playerLoadHandler],
									{
										params ["_timeMultiplier", "_playerLoadHandler"];

										{
											_x enableSimulationGlobal (_x getVariable ["KH_var_originalSimulationState", true]);
										} forEach KH_var_missionSuspensionEntities;

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

										[_playerLoadHandler] call KH_fnc_removeHandler;
										KH_var_missionSuspended = false;
										publicVariable "KH_var_missionSuspended";
									},
									true,
									{KH_var_playersLoaded;},
									false
								] call KH_fnc_execute;
							} forEach allUnits;
						};

						if KH_var_medical then {
							[["CAManBase"], [], KH_fnc_medicalSetup, true] call KH_fnc_entityInit;
						};

						[
							["Man"], 
							[], 
							{
								params ["_unit"];

								[
									[_unit], 
									{
										params ["_unit"];

										[
											[_unit, clientOwner],
											{
												params ["_unit", "_clientOwner"];
												_unit setVariable ["KH_var_owner", _clientOwner, true];
											},
											"SERVER",
											true,
											false
										] call KH_fnc_execute;
									},
									_unit,
									true,
									["PERSISTENT", _unit, [], {}, ""]
								] call KH_fnc_execute;
							}, 
							true
						] call KH_fnc_entityInit;
					},
					true,
					{time > 0;},
					false
				] call KH_fnc_execute;
			}
			else {
				[
					[],
					{
						if KH_var_missionStarted then {
							KH_var_jip = true;
						};

						[
							[],
							{
								if (isNil "KH_var_serverAddress") exitWith {};

								private _networking = [
									missionNamespace,
									[
										"KH_var_networkingPort",
										"KH_var_networkingMaximumMessageSize",
										"KH_var_networkingReceiveBufferSize",
										"KH_var_networkingSendBufferSize",
										"KH_var_networkingConnectionTimeout",
										"KH_var_networkingSendTimeout",
										"KH_var_networkingReceiveTimeout",
										"KH_var_networkingClientStallTimeout",
										"KH_var_networkingKeepAliveTime",
										"KH_var_networkingKeepAliveInterval",
										"KH_var_networkingSendBatchSize",
										"KH_var_networkingCompression",
										"KH_var_networkingCoalesceMessages",
										"KH_var_networkingMaximumCoalesceSize",
										"KH_var_networkingMaximumCoalescedMessages",
										"KH_var_networkingCoalesceDelay"
									],
									"SERVER"
								] call KH_fnc_getRemoteVariable;

								if (isNil "_networking") exitWith {};
								KH_var_networkingSettings = _networking;
								networkInitialize;
								[_handlerId] call KH_fnc_removeHandler;
							},
							true,
							0,
							false
						] call KH_fnc_execute;
					},
					true,
					{KH_var_clientRegistered;},
					false
				] call KH_fnc_execute;
			};
			
			if hasInterface then {
				["KH_eve_playerMissionPreloaded", [clientOwner], "SERVER", false] call KH_fnc_triggerCbaEvent;
				
				[
					[],
					{
						"KH_var_displayLayer" cutRsc ["KH_ResourceKHDisplay", "PLAIN", -1, true, true];

						{
							call _x;
						} forEach KH_var_playerMissionLoadStack;

						call KH_fnc_playerMissionLoadInit;

						[
							[],
							{
								{
									call _x;
								} forEach KH_var_playerLoadStack;

								call KH_fnc_playerLoadInit;
								["KH_eve_playerLoaded", [clientOwner, getPlayerUID player, getPlayerID player, player, [player, true] call KH_fnc_getEntityVariableName], "GLOBAL", false] call KH_fnc_triggerCbaEvent;				

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

										{
											[_corpse] call _x;
										} forEach KH_var_playerRespawnStack;

										[_corpse] call KH_fnc_playerRespawnInit;
										["KH_eve_playerRespawned", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _corpse], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
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

										{
											[_killer, _instigator] call _x;
										} forEach KH_var_playerKilledStack;

										[_killer, _instigator] call KH_fnc_playerKilledInit;
										["KH_eve_playerKilled", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _killer, _instigator], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
									}
								] call KH_fnc_addEventHandler;

								[
									[false, true],
									"Inventory",
									[],
									{
										private _currentTarget = ([] call KH_fnc_getHeadViewTarget) select 4;

										if !(missionNamespace isNil "KH_var_remoteInventoryHandler") then {
											[KH_var_remoteInventoryHandler] call KH_fnc_removeHandler;
											closeDialog 2;
										};

										_caller action ["Gear", _currentTarget];

										missionNamespace setVariable [
											"KH_var_remoteInventoryHandler",
											[
												[_caller, _currentTarget, false],
												{
													params ["_caller", "_currentTarget", "_inventoryOpened"];
													private _inventoryDisplay = findDisplay 602;

													if (isNull _inventoryDisplay) then {
														if _inventoryOpened then {
															[_handlerId] call KH_fnc_removeHandler;
														};
													}
													else {
														if !_inventoryOpened then {
															_this set [2, true];
														};

														if (((_currentTarget distance _caller) > 5) || !(alive _currentTarget)) then {
															closeDialog 2;
															[_handlerId] call KH_fnc_removeHandler;
														};
													};
												},
												true,
												0,
												false
											] call KH_fnc_execute
										];
									},
									{
										if !KH_var_allowRemoteInventories exitWith {
											false;
										};
										
										private _headViewTarget = [] call KH_fnc_getHeadViewTarget;
										private _currentTargetDistance = _headViewTarget select 1;
										private _currentTarget = _headViewTarget select 4;
										((_currentTarget isKindOf "Man") && (alive _currentTarget) && (_currentTargetDistance <= 2));
									},
									true,
									false,
									true,
									false,
									0,
									false,
									false,
									true,
									false,
									true,
									"",
									"",
									[false, true]
								] call KH_fnc_addAction;

								[player, "KH_var_fuelSiphonHolding", objNull, true, true] call KH_fnc_setRespawnVariable;

								[
									[false, true],
									"",
									[],
									[
										{
											private _headViewTarget = [] call KH_fnc_getHeadViewTarget;
											private _currentTargetDistance = _headViewTarget select 1;
											private _currentTarget = _headViewTarget select 4;

											if ((!("Toolkit" in (items _caller)) && KH_var_fuelSiphoningRequireToolkit) || (!([_caller getUnitTrait "Engineer"] call KH_fnc_parseBoolean) && KH_var_fuelSiphoningEngineerOnly)) then {
												if !(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull])) then {
													private _oldTarget = _caller getVariable ["KH_var_fuelSiphonHolding", objNull];
													_caller setVariable ["KH_var_fuelSiphonHolding", objNull, true];
													_oldTarget setVariable ["KH_var_fuelSiphonNozzleHeld", false, true];
													_oldTarget;
												};
											}
											else {
												if (
													(((_caller getVariable ["KH_var_fuelSiphonHolding", objNull]) isEqualTo _currentTarget) && (isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) && !(isNull _currentTarget)) ||
													(!(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull])) && (isNull _currentTarget))
												) then {
													private _oldTarget = _caller getVariable ["KH_var_fuelSiphonHolding", objNull];
													_caller setVariable ["KH_var_fuelSiphonHolding", objNull, true];
													_oldTarget setVariable ["KH_var_fuelSiphonNozzleHeld", false, true];
													_oldTarget;
												}
												else {
													if !(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull])) then {
														if (
															(isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) && 
															(isNull ((_caller getVariable ["KH_var_fuelSiphonHolding", objNull]) getVariable ["KH_var_fuelSiphonAttached", objNull])) && 
															!(isNull _currentTarget) && 
															(_currentTargetDistance <= 2) && 
															(alive _currentTarget) && 
															(((_currentTarget isKindOf "Air") || (_currentTarget isKindOf "LandVehicle") || (_currentTarget isKindOf "Ship")) && !(_currentTarget isKindOf "StaticWeapon"))
														) then {
															private _siphonedTarget = _caller getVariable ["KH_var_fuelSiphonHolding", objNull];
															_currentTarget setVariable ["KH_var_fuelSiphonNozzle", _siphonedTarget getVariable ["KH_var_fuelSiphonNozzle", objNull], true];
															_currentTarget setVariable ["KH_var_fuelSiphonAttached", _siphonedTarget, true];
															_siphonedTarget setVariable ["KH_var_fuelSiphonAttached", _currentTarget, true];
															_siphonedTarget setVariable ["KH_var_fuelSiphonNozzleHeld", false, true];
															_caller setVariable ["KH_var_fuelSiphonHolding", objNull, true];
															[_siphonedTarget, _currentTarget];
														}
														else {
															private _oldTarget = _caller getVariable ["KH_var_fuelSiphonHolding", objNull];
															_caller setVariable ["KH_var_fuelSiphonHolding", objNull, true];
															_oldTarget setVariable ["KH_var_fuelSiphonNozzleHeld", false, true];
															_oldTarget;
														};
													}
													else {
														if (
															(_currentTargetDistance <= 2) && 
															!(isNull _currentTarget) && 
															(alive _currentTarget) && 
															(((_currentTarget isKindOf "Air") || (_currentTarget isKindOf "LandVehicle") || (_currentTarget isKindOf "Ship")) && !(_currentTarget isKindOf "StaticWeapon"))
														) then {
															if (isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) then {
																_caller setVariable ["KH_var_fuelSiphonHolding", _currentTarget, true];
																private _nozzleTip = createVehicle ["Land_Can_V1_F", _currentTarget, [], 0, "CAN_COLLIDE"];
																private _nozzle = ropeCreate [_currentTarget, _currentTarget worldToModelVisual (unitAimPositionVisual _currentTarget), _nozzleTip, [0, 0, 0], 15, ["", [0, 0, -1]], ["", [0, 0, -1]], "KH_FuelSiphon", 63];

																[
																	[_caller, _currentTarget, _nozzleTip, _nozzle],
																	{
																		params ["_caller", "_currentTarget", "_nozzleTip", "_nozzle"];
																		_nozzleTip hideObjectGlobal true;

																		[
																			_this,
																			{
																				params ["_caller", "_currentTarget", "_nozzleTip", "_nozzle"];

																				if ((!(alive _caller) || !(isNull (objectParent _caller)) || (isNull _caller) || (isNull _nozzleTip) || (isNull _nozzle)) || !(isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull]))) then {
																					if (isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) then {
																						_caller setVariable ["KH_var_fuelSiphonHolding", objNull, true];
																						_currentTarget setVariable ["KH_var_fuelSiphonNozzleHeld", false, true];
																						deleteVehicle _nozzleTip;
																						ropeDestroy _nozzle;
																						[_handlerId] call KH_fnc_removeHandler;
																					}
																					else {
																						[_handlerId] call KH_fnc_removeHandler;
																					};
																				};
																			},
																			true,
																			0,
																			false
																		] call KH_fnc_execute;
																	},
																	"SERVER",
																	true,
																	false
																] call KH_fnc_execute;

																_nozzleTip attachTo [_caller, [0, 0, 0], "rightHand", true];
																_currentTarget setVariable ["KH_var_fuelSiphonNozzle", [_nozzleTip, _nozzle], true];
																_currentTarget setVariable ["KH_var_fuelSiphonNozzleHeld", true, true];
																objNull;
															}
															else {
																(_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull]) setVariable ["KH_var_fuelSiphonAttached", objNull, true];
																_currentTarget setVariable ["KH_var_fuelSiphonAttached", objNull, true];
																objNull;
															};
														};
													};
												};
											};
										},
										{},
										{},
										{},
										{
											if !(_resultStart isEqualType objNull) then {
												[
													_resultStart,
													{
														params ["_siphonedTarget", "_currentTarget"];
														private _nozzleTip = (_siphonedTarget getVariable ["KH_var_fuelSiphonNozzle", []]) param [0, objNull]; 
														_nozzleTip attachTo [_currentTarget, _currentTarget worldToModelVisual (unitAimPositionVisual _currentTarget), "", false];

														[
															[_siphonedTarget, _currentTarget],
															{
																params ["_siphonedTarget", "_currentTarget"];
																(_siphonedTarget getVariable ["KH_var_fuelSiphonNozzle", []]) params [["_nozzleTip", objNull], ["_nozzle", objNull]];

																if (
																	((_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull]) isEqualTo _siphonedTarget) && 
																	((_siphonedTarget getVariable ["KH_var_fuelSiphonAttached", objNull]) isEqualTo _currentTarget) &&
																	!(isNull _nozzleTip) && 
																	!(isNull _nozzle) &&
																	(_siphonedTarget in (ropesAttachedTo _nozzleTip))
																) then {
																	private _siphonedFuel = ((KH_var_fuelSiphoningSpeed * _totalDelta) min (fuel _siphonedTarget)) min (1 - (fuel _currentTarget));

																	if (_siphonedFuel isNotEqualTo 0) then {
																		[
																			[_siphonedTarget, _siphonedFuel],
																			{
																				params ["_siphonedTarget", "_siphonedFuel"];
																				_siphonedTarget setFuel ((fuel _siphonedTarget) - _siphonedFuel);
																			},
																			_siphonedTarget,
																			true,
																			false
																		] call KH_fnc_execute;

																		[
																			[_currentTarget, _siphonedFuel],
																			{
																				params ["_currentTarget", "_siphonedFuel"];
																				_currentTarget setFuel ((fuel _currentTarget) + _siphonedFuel);
																			},
																			_currentTarget,
																			true,
																			false
																		] call KH_fnc_execute;
																	};
																}
																else {
																	if !(isNull _siphonedTarget) then {
																		(_siphonedTarget getVariable ["KH_var_fuelSiphonNozzle", []]) params [["_nozzleTip", objNull], ["_nozzle", objNull]];
																		deleteVehicle _nozzleTip;
																		ropeDestroy _nozzle;
																	}
																	else {
																		if !(isNull _currentTarget) then {
																			(_currentTarget getVariable ["KH_var_fuelSiphonNozzle", []]) params [["_nozzleTip", objNull], ["_nozzle", objNull]];
																			deleteVehicle _nozzleTip;
																			ropeDestroy _nozzle;
																		}
																	};

																	_siphonedTarget setVariable ["KH_var_fuelSiphonAttached", objNull, true];
																	_currentTarget setVariable ["KH_var_fuelSiphonAttached", objNull, true];
																	[_handlerId] call KH_fnc_removeHandler;
																};
															},
															true,
															[
																0.1,
																false,
																false,
																{},
																true,
																true
															],
															false
														] call KH_fnc_execute;
													},
													"SERVER",
													true,
													false
												] call KH_fnc_execute;
											}
											else {
												if !(isNull _resultStart) then {
													(_resultStart getVariable ["KH_var_fuelSiphonNozzle", []]) params [["_nozzleTip", objNull], ["_nozzle", objNull]];
													deleteVehicle _nozzleTip;
													ropeDestroy _nozzle;
												};
											};
										},
										{}
									],
									[
										{},
										{
											if !KH_var_fuelSiphoning exitWith {
												false;
											};

											private _headViewTarget = [] call KH_fnc_getHeadViewTarget;
											private _currentTargetDistance = _headViewTarget select 1;
											private _currentTarget = _headViewTarget select 4;

											if (
												(
												(!((_currentTarget isKindOf "Air") || (_currentTarget isKindOf "LandVehicle") || (_currentTarget isKindOf "Ship")) || (_currentTarget isKindOf "StaticWeapon") || !(alive _currentTarget)) &&
												(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull]))
												) ||
												(
												(_currentTarget getVariable ["KH_var_fuelSiphonNozzleHeld", false]) &&
												(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull]))
												) ||
												!(isNull (objectParent _caller))
											) then {
												false;
											}
											else {
												private _text = if ((!("Toolkit" in (items _caller)) && KH_var_fuelSiphoningRequireToolkit) || (!([_caller getUnitTrait "Engineer"] call KH_fnc_parseBoolean) && KH_var_fuelSiphoningEngineerOnly)) then {
													["Drop fuel siphon", ""] select (isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull]));
												}
												else {
													if (
														(((_caller getVariable ["KH_var_fuelSiphonHolding", objNull]) isEqualTo _currentTarget) && (isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) && !(isNull _currentTarget)) ||
														(!(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull])) && (isNull _currentTarget))
													) then {
														"Drop fuel siphon";
													}
													else {
														if !(isNull (_caller getVariable ["KH_var_fuelSiphonHolding", objNull])) then {
															["Drop fuel siphon", "Attach fuel siphon"] select (
																(isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull])) && 
																(isNull ((_caller getVariable ["KH_var_fuelSiphonHolding", objNull]) getVariable ["KH_var_fuelSiphonAttached", objNull])) && 
																!(isNull _currentTarget) && 
																(_currentTargetDistance <= 2) && 
																(alive _currentTarget) && 
																(((_currentTarget isKindOf "Air") || (_currentTarget isKindOf "LandVehicle") || (_currentTarget isKindOf "Ship")) && !(_currentTarget isKindOf "StaticWeapon"))
															);
														}
														else {
															if (
																(_currentTargetDistance <= 2) && 
																!(isNull _currentTarget) && 
																(alive _currentTarget) && 
																(((_currentTarget isKindOf "Air") || (_currentTarget isKindOf "LandVehicle") || (_currentTarget isKindOf "Ship")) && !(_currentTarget isKindOf "StaticWeapon"))
															) then {
																["Detach fuel siphon", "Take fuel siphon"] select (isNull (_currentTarget getVariable ["KH_var_fuelSiphonAttached", objNull]));
															}
															else {
																"";
															};
														};
													};
												};

												if (_text isNotEqualTo "") then {
													_caller setUserActionText [_actionId, _text, _text, ""];
													true;
												}
												else {
													_caller setUserActionText [_actionId, "", "", ""];
													false;
												};
											};
										},
										{true;},
										{true;},
										{true;}
									],
									true,
									true,
									true,
									false,
									0,
									false,
									false,
									true,
									false,
									true,
									"",
									"",
									[false, true]
								] call KH_fnc_addAction;
							},
							true,
							{(!(isNull player) && (alive player));},
							false
						] call KH_fnc_execute;

						[
							[],
							{
								private _medical = [missionNamespace, "KH_var_medical", "SERVER"] call KH_fnc_getRemoteVariable;
								if (isNil "_medical") exitWith {};

								if (_medical param [0, false]) then {
									private _medicalControl = (uiNamespace getVariable ["KH_var_display", displayNull]) ctrlCreate ["RscStructuredText", -1];
									private _medicalControlBackground = (uiNamespace getVariable ["KH_var_display", displayNull]) ctrlCreate ["RscText", -1];
									_medicalControl ctrlShow false;
									_medicalControlBackground ctrlShow false;
									_medicalControlBackground ctrlSetBackgroundColor KH_var_uiBackgroundColor;
									_medicalControlBackground ctrlCommit 0;

									[
										[_medicalControl, _medicalControlBackground],
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
														_control ctrlSetBackgroundColor KH_var_uiEnabledElementColor;
													}
													else {
														private _percentageColor = KH_var_uiEnabledElementColor call BIS_fnc_colorRGBAtoHTML;

														private _position = [
															[KH_var_healthDisplayPercentagePositionX, KH_var_healthDisplayPercentagePositionY, KH_var_healthDisplayPercentageSizeX, KH_var_healthDisplayPercentageSizeY], 
															false
														] call KH_fnc_parseNormalizedScreenTransforms;

														_control ctrlSetPosition _position;
														_controlBackground ctrlSetPosition _position;
														_control ctrlSetAngle [KH_var_healthDisplayPercentageAngle, 0, 0, true];
														_controlBackground ctrlSetAngle [KH_var_healthDisplayPercentageAngle, 0, 0, true];
														_control ctrlSetStructuredText (parseText (["<t align='center' valign='middle' size='", KH_var_healthDisplayPercentageTextSize, "' color='", _percentageColor, "' font='EtelkaMonospaceProBold'>", ((1 - (damage KH_var_playerUnit)) * 100) toFixed 0, "%</t>"] joinString ""));
														_control ctrlSetBackgroundColor KH_var_uiBackgroundColor;
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

								[_handlerId] call KH_fnc_removeHandler;
							},
							true,
							0,
							false
						] call KH_fnc_execute;
					},
					true,
					{KH_var_clientRegistered;},
					false
				] call KH_fnc_execute;
			};

			if (!isServer && !hasInterface) then {
				["KH_eve_headlessMissionPreloaded", [clientOwner], "SERVER", false] call KH_fnc_triggerCbaEvent;

				[
					[],
					{
						{
							call _x;
						} forEach KH_var_headlessMissionLoadStack;

						call KH_fnc_headlessMissionLoadInit;

						[
							[],
							{
								{
									call _x;
								} forEach KH_var_headlessLoadStack;

								call KH_fnc_headlessLoadInit;
								["KH_eve_headlessLoaded", [clientOwner, getPlayerID player, player, [player, true] call KH_fnc_getEntityVariableName], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
							},
							true,
							{(!(isNull player) && (alive player));},
							false
						] call KH_fnc_execute;
					},
					true,
					{KH_var_clientRegistered;},
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
		},
		true,
		"-1",
		false
	] call KH_fnc_execute;
};

nil;