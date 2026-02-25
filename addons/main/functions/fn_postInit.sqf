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

						[
							["Man"],
							[],
							{
								params ["_unit"];
								[[_unit, ""], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
								
								{
									[
										["ENTITY", _unit, "PERSISTENT"],
										_x,
										[[getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures"), getText ((configOf _unit) >> "moves")] select (_x isEqualTo "AnimStateChanged"), _x isEqualTo "AnimStateChanged"],
										{
											params ["_unit", "_animation"];
											_args params ["_animationType", "_isMove"];

											if ((getNumber (configFile >> _animationType >> "states" >> _animation >> "kh_melee")) isEqualTo 1) then {
												if !(_unit getVariable ["KH_var_inMeleeState", false]) then {
													_unit setVariable ["KH_var_inMeleeState", true, true];
												};
										
												private _animationConfig = configFile >> _animationType >> "states" >> _animation;

												if (_unit isNil "KH_var_meleeBlockedUnits") then {
													_unit setVariable ["KH_var_meleeBlockedUnits", [], [[clientOwner, 2], false] select isServer];
												};

												if (_unit isNil "KH_var_meleeParriedUnits") then {
													_unit setVariable ["KH_var_meleeParriedUnits", [], [[clientOwner, 2], false] select isServer];
												};

												[
													[
														_unit,
														_animation,
														_isMove,
														configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]),
														[],
														[],
														[],
														[],
														[],
														_unit getVariable "KH_var_meleeBlockedUnits",
														_unit getVariable "KH_var_meleeParriedUnits",
														getArray (_animationConfig >> "kh_meleeHitTiming"), 
														getArray (_animationConfig >> "kh_meleeBlockTiming"), 
														getArray (_animationConfig >> "kh_meleeParryTiming"),
														getArray (_animationConfig >> "kh_meleeKickTiming"),
														getArray (_animationConfig >> "kh_meleeTackleTiming"),
														getArray (_animationConfig >> "kh_meleeDodgeTiming"),
														getArray (_animationConfig >> "kh_meleeSoundTiming")
													],
													{
														params [
															"_unit", 
															"_animation", 
															"_isMove", 
															"_meleeTypeConfig",
															"_handledHit", 
															"_handledKick", 
															"_handledTackle",
															"_previousBlocks",
															"_previousParries",
															"_blockedUnits", 
															"_parriedUnits", 
															"_hitTiming", 
															"_blockTiming", 
															"_parryTiming", 
															"_kickTiming", 
															"_tackleTiming",
															"_dodgeTiming",
															"_soundTiming"
														];

														if (_animation isNotEqualTo ([gestureState _unit, animationState _unit] select _isMove)) exitWith {
															if (_blockTiming isNotEqualTo []) then {
																_unit setVariable ["KH_var_meleeBlockedUnits", [], [[clientOwner, 2], false] select isServer];
																_unit setVariable ["KH_var_currentMeleeBlock", "", [[clientOwner, 2], false] select isServer];
															};

															if (_parryTiming isNotEqualTo []) then {
																_unit setVariable ["KH_var_meleeParriedUnits", [], [[clientOwner, 2], false] select isServer];
																_unit setVariable ["KH_var_currentMeleeParry", "", [[clientOwner, 2], false] select isServer];
															};

															if (_kickTiming isNotEqualTo []) then {
																_unit setVariable ["KH_var_currentMeleeKick", "", [[clientOwner, 2], false] select isServer];
															};

															if (_tackleTiming isNotEqualTo []) then {
																_unit setVariable ["KH_var_currentMeleeTackle", "", [[clientOwner, 2], false] select isServer];
															};

															if (_dodgeTiming isNotEqualTo []) then {
																_unit setVariable ["KH_var_currentMeleeDodgeDirection", -1, [[clientOwner, 2], false] select isServer];
															};

															if (_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", false];
															};

															[_handlerId] call KH_fnc_removeHandler;
														};

														private _deletionsHit = [];
														private _deletionsKick = [];
														private _deletionsSound = [];
														private _nearUnits = _unit nearEntities ["Man", 15];

														if (_unit in _nearUnits) then {
															_nearUnits deleteAt (_nearUnits find _unit);
														};

														private _time = _unit getUnitMovesInfo ([5, 0] select _isMove);

														if (_hitTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};

															private _hitObjects = [];

															{
																_x params ["_start", "_origin", "_point", "_attack", "_unique"];
																_unique = [_unique, false] call KH_fnc_parseBoolean;

																if (_time >= _start) then {
																	_unit setFatigue (
																		(
																			(getFatigue _unit) + 
																			(
																		 		((getNumber (_meleeTypeConfig >> _attack >> "cost")) / (count _hitTiming)) * 
																		 		KH_var_meleeAbsoluteAttackStaminaConsumptionMultiplier * 
																		 		KH_var_meleeAbsoluteStaminaConsumptionMultiplier
																			)
																		) min 1
																	);

																	_point = if (_point isEqualType []) then {
																		_unit modelToWorldVisualWorld _point;
																	}
																	else {
																		_unit modelToWorldVisualWorld (_unit selectionPosition _point);
																	};

																	private _lineIntersections = [
																		if (_origin isEqualType []) then {
																			_unit modelToWorldVisualWorld _origin;
																		}
																		else {
																			_unit modelToWorldVisualWorld (_unit selectionPosition _origin);
																		}, 
																		_point, 
																		[_unit, "TERRAIN"] + (attachedObjects _unit),
																		true, 
																		-1, 
																		"FIRE", 
																		"NONE", 
																		true,
																		[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																	] call KH_fnc_raycast;

																	if (_lineIntersections isNotEqualTo []) then {
																		{
																			if (((_x select 3) in _handledHit) && !_unique) then {
																				continue;
																			};

																			_hitObjects pushBack [_x select 3, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom (((_x select 3) selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _attack];
																			_handledHit pushBackUnique (_x select 3);
																		} forEach _lineIntersections;
																	};

																	private _hitRadius = getNumber (_meleeTypeConfig >> _attack >> "radius");
																	
																	if (_hitRadius isNotEqualTo 0) then {
																		private _radiusIntersections = ([
																			_point, 
																			[_hitRadius, str _hitRadius, _hitRadius],
																			"OVAL",
																			5,
																			[_unit, "TERRAIN"] + (attachedObjects _unit),
																			true, 
																			-1, 
																			"FIRE", 
																			"NONE",
																			true,
																			[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																		] call KH_fnc_raycast3d) select 0;

																		if (_radiusIntersections isNotEqualTo []) then {
																			{
																				if (((_x select 3) in _handledHit) && !_unique) then {
																					continue;
																				};
																				
																				_hitObjects pushBack [_x select 3, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom (((_x select 3) selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _attack];
																				_handledHit pushBackUnique (_x select 3);
																			} forEach _radiusIntersections;
																		};
																	};

																	_deletionsHit pushBack _forEachIndex;
																};
															} forEach _hitTiming;

															{
																_x params ["_object", "_selection", "_position", "_attack"];

																[
																	"KH_eve_meleeInternalGotHit", 
																	[_object, _unit, _selection, _position, _attack],
																	[_object, "SERVER"] select (isPlayer _object),
																	false
																] call KH_fnc_triggerCbaEvent;
															} forEach _hitObjects;
														};

														if (_blockTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};

															{
																_x params ["_start", "_end", "_block"];
																
																if ((_time >= _start) && (_time <= _end)) then {
																	{
																		if ((abs ((((_unit getRelDir _x) + 180) % 360) - 180)) <= (getNumber (_meleeTypeConfig >> _block >> "angle"))) then {
																			_blockedUnits pushBackUnique _x;
																		}
																		else {
																			if (_x in _blockedUnits) then {
																				_blockedUnits deleteAt (_blockedUnits find _x);
																			};
																		};
																	} forEach _nearUnits;

																	if (_previousBlocks isNotEqualTo _blockedUnits) then {
																		_unit setVariable ["KH_var_meleeBlockedUnits", _blockedUnits, 2];
																		_previousBlocks resize 0;
																		_previousBlocks insert [0, _blockedUnits];
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeBlock", ""]) isNotEqualTo _block) then {
																		_unit setVariable ["KH_var_currentMeleeBlock", _block, [[clientOwner, 2], false] select isServer];
																	};
																
																	break;
																}
																else {
																	_blockedUnits resize 0;

																	if (_previousBlocks isNotEqualTo _blockedUnits) then {
																		_unit setVariable ["KH_var_meleeBlockedUnits", [], 2];
																		_previousBlocks resize 0;
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeBlock", ""]) isNotEqualTo "") then {
																		_unit setVariable ["KH_var_currentMeleeBlock", "", [[clientOwner, 2], false] select isServer];
																	};
																};
															} forEach _blockTiming;
														};

														if (_parryTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};

															{
																_x params ["_start", "_end", "_parry"];

																if ((_time >= _start) && (_time <= _end)) then {
																	_unit setFatigue (
																		(
																			(getFatigue _unit) + 
																			(
																		 		((getNumber (_meleeTypeConfig >> _parry >> "cost")) / (count _parryTiming)) * 
																		 		KH_var_meleeAbsoluteParryStaminaConsumptionMultiplier * 
																		 		KH_var_meleeAbsoluteStaminaConsumptionMultiplier
																			)
																		) min 1
																	);

																	{
																		if ((abs ((((_unit getRelDir _x) + 180) % 360) - 180)) <= (getNumber (_meleeTypeConfig >> _parry >> "angle"))) then {
																			_parriedUnits pushBackUnique _x;
																		}
																		else {
																			if (_x in _parriedUnits) then {
																				_parriedUnits deleteAt (_parriedUnits find _x);
																			};
																		};
																	} forEach _nearUnits;

																	if (_previousParries isNotEqualTo _parriedUnits) then {
																		_unit setVariable ["KH_var_meleeParriedUnits", _parriedUnits, 2];
																		_previousParries resize 0;
																		_previousParries insert [0, _parriedUnits];
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeParry", ""]) isNotEqualTo _parry) then {
																		_unit setVariable ["KH_var_currentMeleeParry", _parry, [[clientOwner, 2], false] select isServer];
																	};

																	break;
																}
																else {
																	_parriedUnits resize 0;

																	if (_previousParries isNotEqualTo _parriedUnits) then {
																		_unit setVariable ["KH_var_meleeParriedUnits", [], 2];
																		_previousParries resize 0;
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeParry", ""]) isNotEqualTo "") then {
																		_unit setVariable ["KH_var_currentMeleeParry", "", [[clientOwner, 2], false] select isServer];
																	};
																};
															} forEach _parryTiming;
														};

														if (_kickTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};

															private _kickedObjects = [];

															{
																_x params ["_start", "_point", "_kick"];

																if (_time >= _start) then {
																	_unit setFatigue (
																		(
																			(getFatigue _unit) + 
																			(
																		 		((getNumber (_meleeTypeConfig >> _kick >> "cost")) / (count _kickTiming)) * 
																		 		KH_var_meleeAbsoluteKickStaminaConsumptionMultiplier * 
																		 		KH_var_meleeAbsoluteStaminaConsumptionMultiplier
																			)
																		) min 1
																	);

																	private _kickIntersections = ([
																		if (_point isEqualType []) then {
																			_unit modelToWorldVisualWorld _point;
																		}
																		else {
																			_unit modelToWorldVisualWorld (_unit selectionPosition _point);
																		}, 
																		[0.5, "0.5", 0.5],
																		"OVAL",
																		5,
																		[_unit, "TERRAIN"] + (attachedObjects _unit),
																		true, 
																		-1, 
																		"FIRE", 
																		"NONE",
																		true,
																		[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																	] call KH_fnc_raycast3d) select 0;

																	if (_kickIntersections isNotEqualTo []) then {
																		{
																			if ((_x select 3) in _handledKick) then {
																				continue;
																			};

																			_kickedObjects pushBack [_x select 3, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom (((_x select 3) selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _kick];
																			_handledKick pushBack (_x select 3);
																		} forEach _kickIntersections;
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeKick", ""]) isNotEqualTo _kick) then {
																		_unit setVariable ["KH_var_currentMeleeKick", _kick, [[clientOwner, 2], false] select isServer];
																	};

																	_deletionsKick pushBack _forEachIndex;
																};
															} forEach _kickTiming;

															{
																_x params ["_object", "_selection", "_position", "_kick"];

																[
																	"KH_eve_meleeInternalGotKicked", 
																	[_object, _unit, _selection, _position, _kick],
																	[_object, "SERVER"] select (isPlayer _object),
																	false
																] call KH_fnc_triggerCbaEvent;
															} forEach _kickedObjects;
														};

														if (_tackleTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};

															private _tackledObjects = [];

															{
																_x params ["_start", "_end", "_tackle"];

																if ((_time >= _start) && (_time <= _end)) then {
																	_unit setFatigue (
																		(
																			(getFatigue _unit) + 
																			(
																		 		((getNumber (_meleeTypeConfig >> _tackle >> "cost")) / (count _tackleTiming)) * 
																		 		KH_var_meleeAbsoluteTackleStaminaConsumptionMultiplier * 
																		 		KH_var_meleeAbsoluteStaminaConsumptionMultiplier
																			)
																		) min 1
																	);

																	private _tackleIntersections = ([
																		(AGLToASL (unitAimPositionVisual _unit)) vectorAdd ((vectorDir _unit) vectorMultiply 0.25), 
																		[0.5, 0.5, "1"],
																		"OVAL",
																		5,
																		[_unit, "TERRAIN"] + (attachedObjects _unit),
																		true, 
																		-1, 
																		"FIRE", 
																		"GEOM",
																		true,
																		[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																	] call KH_fnc_raycast3d) select 0;

																	if (_tackleIntersections isNotEqualTo []) then {
																		{
																			if ((_x select 3) in _handledTackle) then {
																				continue;
																			};

																			_tackledObjects pushBack [_x select 3, _tackle];
																			_handledTackle pushBack (_x select 3);
																		} forEach _tackleIntersections;
																	};

																	if ((_unit getVariable ["KH_var_currentMeleeTackle", ""]) isNotEqualTo _tackle) then {
																		_unit setVariable ["KH_var_currentMeleeTackle", _tackle, [[clientOwner, 2], false] select isServer];
																	};

																	break;
																}
																else {
																	if ((_unit getVariable ["KH_var_currentMeleeTackle", ""]) isNotEqualTo "") then {
																		_unit setVariable ["KH_var_currentMeleeTackle", "", [[clientOwner, 2], false] select isServer];
																	};
																};
															} forEach _tackleTiming;

															{
																_x params ["_object", "_tackle"];

																[
																	"KH_eve_meleeInternalGotTackled", 
																	[_object, _unit, _tackle],
																	[_object, "SERVER"] select (isPlayer _object),
																	false
																] call KH_fnc_triggerCbaEvent;
															} forEach _tackledObjects;
														};

														if (_dodgeTiming isNotEqualTo []) then {
															if !(_unit getVariable ["KH_var_meleeMoveActive", false]) then {
																_unit setVariable ["KH_var_meleeMoveActive", true];
															};
															
															{
																_x params ["_start", "_end", "_direction"];

																if ((_time >= _start) && (_time <= _end)) then {
																	_unit setFatigue (
																		(
																			(getFatigue _unit) + 
																			(
																		 		((getNumber ((configOf _unit) >> "kh_meleeDodgeCost")) / (count _dodgeTiming)) * 
																		 		KH_var_meleeAbsoluteDodgeStaminaConsumptionMultiplier * 
																		 		KH_var_meleeAbsoluteStaminaConsumptionMultiplier
																			)
																		) min 1
																	);

																	if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo _tackle) then {
																		_unit setVariable ["KH_var_currentMeleeDodgeDirection", _direction, [[clientOwner, 2], false] select isServer];
																	};

																	break;
																}
																else {
																	if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo "") then {
																		_unit setVariable ["KH_var_currentMeleeDodgeDirection", -1, [[clientOwner, 2], false] select isServer];
																	};
																};
															} forEach _dodgeTiming;
														};

														if (_soundTiming isNotEqualTo []) then {
															{
																_x params ["_start", "_point", "_type", "_sound"];

																if (_time >= _start) then {
																	private _sound = getArray (_meleeTypeConfig >> _type >> "Sounds" >> _sound);

																	if (_sound isNotEqualTo []) then {
																		playSound3D [
																			((getArray (configFile >> "CfgSounds" >> (selectRandom _sound) >> "sound")) select 0) select [1], 
																			_unit, 
																			[insideBuilding _unit, false] call KH_fnc_parseBoolean, 
																			_unit modelToWorldVisualWorld (_unit selectionPosition _point), 
																			1, 
																			1, 
																			100, 
																			0, 
																			false
																		];
																	};

																	_deletionsSound pushBack _forEachIndex;
																};
															} forEach _soundTiming;
														};

														_hitTiming deleteAt _deletionsHit;
														_kickTiming deleteAt _deletionsKick;
														_soundTiming deleteAt _deletionsSound;
													},
													true,
													0,
													false
												] call KH_fnc_execute;
											}
											else {
												if _isMove then {
													if (_unit getVariable ["KH_var_inMeleeState", false]) then {
														_unit setVariable ["KH_var_inMeleeState", false, true];
													};
												};
											};
										}
									] call KH_fnc_addEventHandler;
								} forEach ["AnimStateChanged", "GestureChanged"];

								{
									[
										["ENTITY", _unit, "PERSISTENT"], 
										_x, 
										[], 
										{
											params ["_unit"];
											[_unit, ""] call KH_fnc_updateMeleeState;
										}
									] call KH_fnc_addEventHandler;
								} forEach ["Attached", "Detached", "GetInMan", "GetOutMan", "SlotItemChanged", "WeaponChanged"];
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
								[player, "KH_var_rawMeleeStance", false, false, true] call KH_fnc_setRespawnVariable;

								[
									[false, true],
									"",
									[],
									[
										{
											private _headViewTarget = [] call KH_fnc_getHeadViewTarget;
											private _currentTargetDistance = _headViewTarget select 1;
											private _currentTarget = _headViewTarget select 4;

											if ((!("Toolkit" in (items _caller)) && KH_var_fuelSiphoningRequireToolkit) || (!([_caller getUnitTrait "Engineer", false] call KH_fnc_parseBoolean) && KH_var_fuelSiphoningEngineerOnly)) then {
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
												private _text = if ((!("Toolkit" in (items _caller)) && KH_var_fuelSiphoningRequireToolkit) || (!([_caller getUnitTrait "Engineer", false] call KH_fnc_parseBoolean) && KH_var_fuelSiphoningEngineerOnly)) then {
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

			[] spawn {
				while {true;} do {
					sleep 0.2;

					{
						private _unit = _x;
						private _primaryWeapon = primaryWeapon _unit;
						private _handgunWeapon = handgunWeapon _unit;
						private _secondaryWeapon = secondaryWeapon _unit;
						
						private _meleeWeapon = if ((getText (configFile >> "CfgWeapons" >> _primaryWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
							((_unit weaponsInfo [_primaryWeapon, false]) select 0) select 0;
						}
						else {
							if ((getNumber (configFile >> "CfgWeapons" >> _handgunWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
								((_unit weaponsInfo [_handgunWeapon, false]) select 0) select 0;
							}
							else {
								if ((getNumber (configFile >> "CfgWeapons" >> _secondaryWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
									((_unit weaponsInfo [_secondaryWeapon, false]) select 0) select 0;
								}
								else {
									-1;
								};
							};
						};

						if ((_meleeWeapon isNotEqualTo -1) && ((count ([_primaryWeapon, _handgunWeapon, _secondaryWeapon] select {_x isNotEqualTo "";})) isEqualTo 1)) then {
							[
								[_unit, _meleeWeapon],
								{
									params ["_unit", "_meleeWeapon"];

									if !(_unit getVariable ["KH_var_aiIsMelee", false]) then {
										_unit setVariable ["KH_var_aiIsMelee", true, true];

										_unit setVariable [
											"KH_var_aiPreMeleeFeatures", 
											[
												_unit checkAIFeature "AUTOTARGET",
												_unit checkAIFeature "AIMINGERROR",
												_unit checkAIFeature "TARGET",
												_unit checkAIFeature "WEAPONAIM",
												_unit checkAIFeature "FSM",
												_unit checkAIFeature "AUTOCOMBAT",
												_unit checkAIFeature "COVER",
												_unit checkAIFeature "SUPPRESSION",
												_unit checkAIFeature "FIREWEAPON"
											]
										];

										_unit setVariable [
											"KH_var_aiPreMeleeSkill", 
											[
												_unit skill "aimingAccuracy",
												_unit skill "aimingShake",
												_unit skill "aimingSpeed",
												_unit skill "spotDistance",
												_unit skill "spotTime",
												_unit skill "courage",
												_unit skill "reloadSpeed",
												_unit skill "commanding",
												_unit skill "general"
											]
										];

										_unit setVariable ["KH_var_aiPreMeleeBehaviour", behaviour _unit];
										_unit setVariable ["KH_var_aiPreMeleeCombatMode", combatMode _unit];
										_unit setVariable ["KH_var_aiPreMeleeStance", unitPos _unit];
										_unit enableAIFeature ["AUTOTARGET", false];
										_unit enableAIFeature ["AIMINGERROR", false];
										_unit enableAIFeature ["TARGET", false];
										_unit enableAIFeature ["WEAPONAIM", false];
										_unit enableAIFeature ["FSM", false];
										_unit enableAIFeature ["AUTOCOMBAT", false];
										_unit enableAIFeature ["COVER", false];
										_unit enableAIFeature ["SUPPRESSION", false];
										_unit enableAIFeature ["FIREWEAPON", false];
										_unit setSkill ["aimingAccuracy", 1];
										_unit setSkill ["aimingShake", 1];
										_unit setSkill ["aimingSpeed", 1];
										_unit setSkill ["spotDistance", 0];
										_unit setSkill ["spotTime", 0];
										_unit setSkill ["courage", 1];
										_unit setSkill ["reloadSpeed", 1];
										_unit setSkill ["commanding", 1];
										_unit setSkill ["general", 1];
										_unit setBehaviourStrong "CARELESS";
										_unit setUnitCombatMode "BLUE";
										_unit setUnitPos "UP";
										_unit action ["SwitchWeapon", _unit, _unit, _meleeWeapon];
									};

									private _targets = (_unit targetsQuery [objNull, sideUnknown, "", [], 0]) select {(_x select 2) isNotEqualTo (side (group _unit));};

									if (_targets isNotEqualTo []) then {
										private _closestTarget = ((_targets select 0) select 1);
										_unit move (getPosATL _closestTarget);

										if (((_unit distance _closestTarget) <= 2) && ((abs ((((_unit getRelDir _closestTarget) + 180) % 360) - 180)) <= 22.5)) then {
											[_unit, "ATTACK"] call KH_fnc_updateMeleeState;
										};
									};
								},
								true,
								true,
								false
							] call KH_fnc_execute;
						}
						else {
							[
								[_unit],
								{
									params ["_unit"];

									if (_unit getVariable ["KH_var_aiIsMelee", false]) then {
										_unit setVariable ["KH_var_aiIsMelee", false, true];
										private _aiFeatures = _unit getVariable ["KH_var_aiPreMeleeFeatures", []];
										private _aiSkill = _unit getVariable ["KH_var_aiPreMeleeSkill", []];

										{
											_unit enableAIFeature [_x, _aiFeatures param [_forEachIndex, true]];
										} forEach ["AUTOTARGET", "AIMINGERROR", "TARGET", "WEAPONAIM", "FSM", "AUTOCOMBAT", "COVER", "SUPPRESSION", "FIREWEAPON"];

										{
											_unit setSkill [_x, _aiSkill param [_forEachIndex, 1]];
										} forEach ["aimingAccuracy", "aimingShake", "aimingSpeed", "spotDistance", "spotTime", "courage", "reloadSpeed", "commanding", "general"];

										_unit setBehaviourStrong ((_unit getVariable ["KH_var_aiPreMeleeBehaviour", "SAFE"]));
										_unit setUnitCombatMode ((_unit getVariable ["KH_var_aiPreMeleeCombatMode", "YELLOW"]));
										_unit setUnitPos ((_unit getVariable ["KH_var_aiPreMeleeStance", "UP"]));
										_unit action ["SwitchWeapon", _unit, _unit, 299];
									};
								},
								true,
								true,
								false
							] call KH_fnc_execute;
						};
					} forEach (allUnits select {(!(isPlayer _x) && (isNull (remoteControlled _x)) && (local _x));});
				};
			};
		},
		true,
		"-1",
		false
	] call KH_fnc_execute;
};

nil;