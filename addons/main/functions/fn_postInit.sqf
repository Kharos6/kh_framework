isNil {
	if KH_var_isMenu exitWith {};

	[
		[],
		{
			KH_var_missionLoaded = true;

			if isServer then {
				KH_var_meleeDodgeFailureAngleRange = KH_var_meleeDodgeFailureAngleRange / 2;
				publicVariable "KH_var_meleeDodgeFailureAngleRange";

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

						KH_var_missionInitialized = true;
						publicVariable "KH_var_missionInitialized";

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
								if (_unit getVariable ["KH_var_meleeSetupComplete", false]) exitWith {};
								_unit setVariable ["KH_var_meleeSetupComplete", true, true];
								
								{
									[
										["ENTITY", _unit, "PERSISTENT"],
										_x,
										[
											[getText (configFile >> (getText ((configOf _unit) >> "moves")) >> "gestures"), getText ((configOf _unit) >> "moves")] select (_x isEqualTo "AnimStateChanged"), 
											_x isEqualTo "AnimStateChanged"
										],
										{
											params ["_unit", "_animation"];
											_args params ["_animationType", "_isMove"];
											private _animationConfig = configFile >> _animationType >> "states" >> _animation;

											if ((getNumber (_animationConfig >> "kh_traversalTeleport")) isEqualTo 1) then {
												if ((_unit getVariable ["KH_var_traversalTarget", []]) isNotEqualTo []) then {
													_unit setPosASL (_unit getVariable ["KH_var_traversalTarget", getPosASLVisual _unit]);
													_unit setVariable ["KH_var_traversalTarget", []];
												};
											};

											if ((getNumber (_animationConfig >> "kh_meleeHasAction")) isEqualTo 1) then {
												private _handlerType = ["KH_var_meleeStateHandler", "KH_var_meleeGestureHandler"] select _isMove;

												if !(_unit isNil _handlerType) then {
													[_unit getVariable _handlerType] call KH_fnc_removeHandler;
												};
												
												private _clientType = [[clientOwner, 2], false] select isServer;

												if _isMove then {
													if ((getNumber (_animationConfig >> "minPlayTime")) isNotEqualTo 0) then {
														_unit setVariable ["KH_var_meleeMoveActive", true];
													}
													else {
														_unit setVariable ["KH_var_meleeMoveActive", false];
													};
												}
												else {
													if ((getNumber (_animationConfig >> "minPlayTime")) isNotEqualTo 0) then {
														_unit setVariable ["KH_var_meleeGestureActive", true];
													}
													else {
														_unit setVariable ["KH_var_meleeGestureActive", false];
													};
												};

												_unit setVariable ["KH_var_meleeBlockedUnits", [], _clientType];
												_unit setVariable ["KH_var_currentMeleeBlock", "", _clientType];
												_unit setVariable ["KH_var_meleeParriedUnits", [], _clientType];
												_unit setVariable ["KH_var_currentMeleeParry", "", _clientType];
												_unit setVariable ["KH_var_currentMeleeKick", "", _clientType];
												_unit setVariable ["KH_var_currentMeleeTackle", "", _clientType];
												_unit setVariable ["KH_var_currentMeleeDodgeDirection", -1, _clientType];
												private _moveVector = [0, 0, 0];
												private _moveThreshold = [];

												private _avoidCollision = if (!(isPlayer _unit) && _isMove && KH_var_meleeAiCollisionDetection) then {
													_moveVector = _unit vectorModelToWorldVisual ((_unit getUnitMovesInfo 4) vectorMultiply -1);
													private _aimPosition = AGLToASL (unitAimPositionVisual _unit);
													private _positionAsl = getPosASLVisual _unit;
													_aimPosition set [0, _positionAsl select 0];
													_aimPosition set [1, _positionAsl select 1];

													private _moveIntersection = ([
														_aimPosition,
														_aimPosition vectorAdd _moveVector, 
														[_unit, "TERRAIN"] + (attachedObjects _unit),
														true, 
														1, 
														"GEOM", 
														"FIRE", 
														true,
														[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
													] call KH_fnc_raycast) param [0, []];

													if (_moveIntersection isNotEqualTo []) then {
														private _entity = _moveIntersection select 3;

														if !(isNull _entity) then {
															if !(isPlayer _entity) then {
																_moveThreshold = (_moveIntersection select 0) vectorAdd ((vectorNormalized _moveVector) vectorMultiply -0.45);
																true;
															}
															else {
																false;
															};
														}
														else {
															false;
														};
													}
													else {
														false;
													};
												}
												else {
													false;
												};

												_unit setVariable [
													_handlerType,
													[
														[
															_unit,
															_animation,
															_clientType,
															getNumber (_animationConfig >> "minPlayTime"),
															_isMove,
															_moveVector,
															_moveThreshold,
															getPosWorldVisual _unit,
															_avoidCollision,
															false,
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
															getArray (_animationConfig >> "kh_meleeSoundTiming"),
															false,
															false,
															false,
															false,
															false
														],
														{
															params [
																"_unit", 
																"_animation",
																"_clientType",
																"_exclusiveTime",
																"_isMove",
																"_moveVector",
																"_moveThreshold",
																"_correctedPosition",
																"_avoidCollision",
																"_collided",
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
																"_soundTiming",
																"_hitFatigue",
																"_parryFatigue",
																"_kickFatigue",
																"_tackleFatigue",
																"_dodgeFatigue"
															];

															if (_animation isNotEqualTo ([gestureState _unit, animationState _unit] select _isMove)) exitWith {
																[_handlerId] call KH_fnc_removeHandler;
															};

															private _time = _unit getUnitMovesInfo ([5, 0] select _isMove);

															if !_isMove then {
																if (_time >= _exclusiveTime) then {
																	_unit setVariable ["KH_var_meleeGestureActive", false];
																};
															}
															else {
																if (_time >= _exclusiveTime) then {
																	_unit setVariable ["KH_var_meleeMoveActive", false];
																};
															};

															private _vectorCorrected = false;
															private _deletionsHit = [];
															private _deletionsKick = [];
															private _deletionsSound = [];

															private _nearUnits = if ((_blockTiming isNotEqualTo []) || (_parryTiming isNotEqualTo [])) then {
																(_unit nearEntities ["Man", 10]) - [_unit];
															}
															else {
																[];
															};

															if (_hitTiming isNotEqualTo []) then {
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
																};

																private _hitObjects = [];

																{
																	_x params ["_start", "_origin", "_point", "_attack", "_unique"];
																	_unique = [_unique, false] call KH_fnc_parseBoolean;

																	if !_hitFatigue then {
																		_unit setFatigue (
																			(
																				(getFatigue _unit) + 
																				(
																					(getNumber (_meleeTypeConfig >> _attack >> "cost")) * 
																					KH_var_meleeAbsoluteAttackStaminaConsumptionMultiplier * 
																					KH_var_meleeAbsoluteStaminaConsumptionMultiplier *
																					([KH_var_meleeAbsoluteAiStaminaConsumptionMultiplier, KH_var_meleeAbsolutePlayerStaminaConsumptionMultiplier] select (isPlayer _unit))
																				)
																			) min 1
																		);

																		_this set [25, true];
																	};

																	if (_time >= _start) then {
																		_point = if (_point isEqualType []) then {
																			_unit modelToWorldVisualWorld (_point vectorAdd (getArray (_meleeTypeConfig >> _attack >> "rangeOffset")));
																		}
																		else {
																			_unit modelToWorldVisualWorld ((_unit selectionPosition _point) vectorAdd (getArray (_meleeTypeConfig >> _attack >> "rangeOffset")));
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
																			["NONE", "GEOM"] select KH_var_allowGeometryHitDetection, 
																			true,
																			[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																		] call KH_fnc_raycast;

																		if (_lineIntersections isNotEqualTo []) then {
																			{
																				private _object = _x select 3;

																				if (((_object in _handledHit) && !_unique) || (((side (group _object)) isEqualTo (side (group _unit))) && KH_var_meleeAttackIgnoreFriendlies)) then {
																					continue;
																				};

																				_hitObjects pushBack [_object, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom ((_object selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _attack];
																				_handledHit pushBackUnique _object;
																			} forEach _lineIntersections;
																		};

																		private _hitRadiusConfig = _meleeTypeConfig >> _attack >> "radius";

																		private _hitRadius = if (isNumber _hitRadiusConfig) then {
																			private _radius = getNumber _hitRadiusConfig;

																			if (_radius isEqualTo 0) then {
																				[];
																			}
																			else {
																				[_radius, _radius, str _radius];
																			};
																		}
																		else {
																			getArray _hitRadiusConfig;
																		};
																		
																		if (_hitRadius isNotEqualTo []) then {
																			private _radiusIntersections = ([
																				_point, 
																				[vectorDir _unit, vectorUp _unit],
																				_hitRadius,
																				"OVAL",
																				6,
																				[_unit, "TERRAIN"] + (attachedObjects _unit),
																				true, 
																				-1, 
																				"FIRE", 
																				["NONE", "GEOM"] select KH_var_allowGeometryHitDetection,
																				true,
																				[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																			] call KH_fnc_raycast3d) select 0;

																			if (_radiusIntersections isNotEqualTo []) then {
																				{
																					private _object = _x select 3;

																					if (((_object in _handledHit) && !_unique) || (((side (group _object)) isEqualTo (side (group _unit))) && KH_var_meleeAttackIgnoreFriendlies)) then {
																						continue;
																					};
																					
																					_hitObjects pushBack [_object, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom ((_object selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _attack];
																					_handledHit pushBackUnique _object;
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
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
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
																			_unit setVariable ["KH_var_currentMeleeBlock", _block, _clientType];
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
																			_unit setVariable ["KH_var_currentMeleeBlock", "", _clientType];
																		};
																	};
																} forEach _blockTiming;
															};

															if (_parryTiming isNotEqualTo []) then {
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
																};

																{
																	_x params ["_start", "_end", "_parry"];

																	if !_parryFatigue then {
																		_unit setFatigue (
																			(
																				(getFatigue _unit) + 
																				(
																					(getNumber (_meleeTypeConfig >> _parry >> "cost")) * 
																					KH_var_meleeAbsoluteParryStaminaConsumptionMultiplier * 
																					KH_var_meleeAbsoluteStaminaConsumptionMultiplier *
																					([KH_var_meleeAbsoluteAiStaminaConsumptionMultiplier, KH_var_meleeAbsolutePlayerStaminaConsumptionMultiplier] select (isPlayer _unit))
																				)
																			) min 1
																		);

																		_this set [26, true];
																	};

																	if ((_time >= _start) && (_time <= _end)) then {
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
																			_unit setVariable ["KH_var_currentMeleeParry", _parry, _clientType];
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
																			_unit setVariable ["KH_var_currentMeleeParry", "", _clientType];
																		};
																	};
																} forEach _parryTiming;
															};

															if (_kickTiming isNotEqualTo []) then {
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
																};

																private _kickedObjects = [];

																{
																	_x params ["_start", "_point", "_kick"];

																	if !_kickFatigue then {
																		_unit setFatigue (
																			(
																				(getFatigue _unit) + 
																				(
																					(getNumber (_meleeTypeConfig >> _kick >> "cost")) * 
																					KH_var_meleeAbsoluteKickStaminaConsumptionMultiplier * 
																					KH_var_meleeAbsoluteStaminaConsumptionMultiplier *
																					([KH_var_meleeAbsoluteAiStaminaConsumptionMultiplier, KH_var_meleeAbsolutePlayerStaminaConsumptionMultiplier] select (isPlayer _unit))
																				)
																			) min 1
																		);

																		_this set [27, true];
																	};

																	if (_time >= _start) then {
																		private _kickIntersections = ([
																			if (_point isEqualType []) then {
																				_unit modelToWorldVisualWorld _point;
																			}
																			else {
																				_unit modelToWorldVisualWorld (_unit selectionPosition _point);
																			},
																			[vectorDir _unit, vectorUp _unit],
																			[0.3, 0.3, "0.3"],
																			"OVAL",
																			6,
																			[_unit, "TERRAIN"] + (attachedObjects _unit),
																			true, 
																			-1, 
																			"FIRE", 
																			["NONE", "GEOM"] select KH_var_allowGeometryHitDetection,
																			true,
																			[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																		] call KH_fnc_raycast3d) select 0;

																		if (_kickIntersections isNotEqualTo []) then {
																			{
																				private _object = _x select 3;

																				if ((_object in _handledKick) || (((side (group _object)) isEqualTo (side (group _unit))) && KH_var_meleeKickIgnoreFriendlies)) then {
																					continue;
																				};

																				_kickedObjects pushBack [_object, ((_x select 4) select {"hit" in _x;}) param [0, selectRandom ((_object selectionNames "FireGeometry") select {"hit" in _x;})], _x select 0, _kick];
																				_handledKick pushBack _object;
																			} forEach _kickIntersections;
																		};

																		if ((_unit getVariable ["KH_var_currentMeleeKick", ""]) isNotEqualTo _kick) then {
																			_unit setVariable ["KH_var_currentMeleeKick", _kick, _clientType];
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
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
																};

																private _tackledObjects = [];

																{
																	_x params ["_start", "_end", "_tackle"];

																	if !_tackleFatigue then {
																		_unit setFatigue (
																			(
																				(getFatigue _unit) + 
																				(
																					(getNumber (_meleeTypeConfig >> _tackle >> "cost")) * 
																					KH_var_meleeAbsoluteTackleStaminaConsumptionMultiplier * 
																					KH_var_meleeAbsoluteStaminaConsumptionMultiplier *
																					([KH_var_meleeAbsoluteAiStaminaConsumptionMultiplier, KH_var_meleeAbsolutePlayerStaminaConsumptionMultiplier] select (isPlayer _unit))
																				)
																			) min 1
																		);

																		_this set [28, true];
																	};

																	if ((_time >= _start) && (_time <= _end)) then {
																		private _aimPosition = (AGLToASL (unitAimPositionVisual _unit)) vectorAdd ((vectorDir _unit) vectorMultiply 0.25);

																		private _lineIntersections = [
																			_aimPosition, 
																			_aimPosition vectorAdd ((vectorDir _unit) vectorMultiply 0.5), 
																			[_unit, "TERRAIN"] + (attachedObjects _unit),
																			true, 
																			-1, 
																			"FIRE", 
																			"GEOM", 
																			true,
																			[[], ["LINE", [], 1]] select KH_var_meleeDebugMode
																		] call KH_fnc_raycast;

																		if (_lineIntersections isNotEqualTo []) then {
																			{
																				private _object = _x select 3;

																				if ((_object in _handledTackle) || (((side (group _object)) isEqualTo (side (group _unit))) && KH_var_meleeTackleIgnoreFriendlies)) then {
																					continue;
																				};

																				_tackledObjects pushBack [_object, _tackle];
																				_handledTackle pushBack _object;
																			} forEach _lineIntersections;
																		};

																		private _tackleIntersections = ([
																			_aimPosition, 
																			[vectorDir _unit, vectorUp _unit],
																			[0.5, 0.5, "0.75"],
																			"OVAL",
																			6,
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
																				private _object = _x select 3;

																				if ((_object in _handledTackle) || (((side (group _object)) isEqualTo (side (group _unit))) && KH_var_meleeTackleIgnoreFriendlies)) then {
																					continue;
																				};

																				_tackledObjects pushBack [_x select 3, _tackle];
																				_handledTackle pushBack _object;
																			} forEach _tackleIntersections;
																		};

																		if ((_unit getVariable ["KH_var_currentMeleeTackle", ""]) isNotEqualTo _tackle) then {
																			_unit setVariable ["KH_var_currentMeleeTackle", _tackle, _clientType];
																		};

																		break;
																	}
																	else {
																		if ((_unit getVariable ["KH_var_currentMeleeTackle", ""]) isNotEqualTo "") then {
																			_unit setVariable ["KH_var_currentMeleeTackle", "", _clientType];
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
																if !_vectorCorrected then {
																	if _avoidCollision then {
																		if _collided then {
																			_unit setPosWorld _correctedPosition;
																		}
																		else {
																			private _currentPosition = getPosWorldVisual _unit;

																			if (((_currentPosition vectorDiff _moveThreshold) vectorDotProduct (vectorNormalized _moveVector)) >= 0) then {
																				private _direction = vectorNormalized _moveVector;
																				private _correction = _currentPosition vectorAdd (_direction vectorMultiply -((_currentPosition vectorDiff _moveThreshold) vectorDotProduct _direction));
																				_unit setPosWorld _correction;
																				_this set [9, true];
																				_this set [7, _correction];
																			};
																		};

																		_vectorCorrected = true;
																	};
																};
																
																{
																	_x params ["_start", "_end", "_direction", "_dodge"];

																	if !_dodgeFatigue then {
																		_unit setFatigue (
																			(
																				(getFatigue _unit) + 
																				(
																					(getNumber (_meleeTypeConfig >> _dodge >> "cost")) * 
																					KH_var_meleeAbsoluteDodgeStaminaConsumptionMultiplier * 
																					KH_var_meleeAbsoluteStaminaConsumptionMultiplier *
																					([KH_var_meleeAbsoluteAiStaminaConsumptionMultiplier, KH_var_meleeAbsolutePlayerStaminaConsumptionMultiplier] select (isPlayer _unit))
																				)
																			) min 1
																		);
																		
																		_this set [29, true];
																	};

																	if ((_time >= _start) && (_time <= _end)) then {
																		if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo _direction) then {
																			_unit setVariable ["KH_var_currentMeleeDodgeDirection", _direction, _clientType];
																		};

																		break;
																	}
																	else {
																		if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo "") then {
																			_unit setVariable ["KH_var_currentMeleeDodgeDirection", -1, _clientType];
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
																				(insideBuilding _unit) >= 0.5, 
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
													] call KH_fnc_execute
												];
											}
											else {
												private _activeType = ["KH_var_meleeGestureActive", "KH_var_meleeMoveActive"] select _isMove;

												if (_unit getVariable [_activeType, false]) then {
													_unit setVariable [_activeType, false];
												};
											};
										}
									] call KH_fnc_addEventHandler;
								} forEach ["AnimStateChanged", "GestureChanged"];

								[
									["ENTITY", _unit, "PERSISTENT"],
									"GestureDone",
									[],
									{
										params ["_unit"];
										_unit setVariable ["KH_var_meleeGestureActive", false];
										_unit setVariable ["KH_var_attackGestureIndex", -1];
									}
								] call KH_fnc_addEventHandler;

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
				"KH_var_displayLayer" cutRsc ["KH_ResourceKHDisplay", "PLAIN", -1, true, true];

				[
					[],
					{
						with uiNamespace do {
							KH_var_loadingDisplay = ["RscText", "LOADING...", [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
						};
					},
					true,
					{CBA_missionTime > 0;},
					false
				] call KH_fnc_execute;

				[
					[],
					{
						if KH_var_missionStarted then {
							KH_var_jip = true;
						};

						[
							[],
							{
								with uiNamespace do {
									ctrlDelete KH_var_loadingDisplay;
									KH_var_loadingDisplay = nil;
								};
							},
							true,
							{!(uiNamespace isNil "KH_var_loadingDisplay");},
							false
						] call KH_fnc_execute;

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
								khNetworkInitialize;
								[_handlerId] call KH_fnc_removeHandler;
							},
							true,
							0,
							false
						] call KH_fnc_execute;
					},
					true,
					{KH_var_clientRegistered && KH_var_missionInitialized;},
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
										_corpse setVariable ["KH_var_playerUnit", _unit, true];
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

											if ((KH_var_healthDisplay isNotEqualTo "NONE") && (KH_var_playerUnit getVariable ["KH_var_medicalInitComplete", false]) && !visibleMap && !dialog && (isNull curatorCamera)) then {
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
					{KH_var_clientRegistered && KH_var_missionInitialized;},
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
								KH_var_playerUnit = player;
								player setVariable ["KH_var_playerUnit", KH_var_playerUnit, true];

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
					{KH_var_clientRegistered && KH_var_missionInitialized;},
					false
				] call KH_fnc_execute;
			};

			[
				[], 
				["Animal"], 
				{
					params ["_entity"];

					if (local _entity) then {
						KH_var_allLocalEntities pushBackUnique _entity;
					};

					[
						["ENTITY", _entity, "LOCAL"],
						"Local",
						[],
						{
							params ["_entity", "_isLocal"];

							if _isLocal then {
								KH_var_allLocalEntities pushBackUnique _entity;
							}
							else {
								if (_entity in KH_var_allLocalEntities) then {
									KH_var_allLocalEntities deleteAt (KH_var_allLocalEntities find _entity);
								};
							};
						}
					] call KH_fnc_addEventHandler;
				}, 
				true
			] call KH_fnc_entityInit;

			{
				_x params ["_arguments", ["_function", {}, [{}]], ["_delay", 0, [0, {}]]];

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
					
					if (_delay isEqualType 0) then {
						_delay = {
							params ["_delay"];
							time > _delay;
						};
					};

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
						_delay, 
						false
					] call KH_fnc_execute;
				};
			} forEach KH_var_postInitExecutions;

			{
				luaExecute _x;
			} forEach KH_var_postInitLuaExecutions;

			[] spawn {
				waitUntil {time > 1;};

				while {true;} do {
					sleep 0.2;

					if KH_var_allowAiMelee then {
						{
							private _unit = _x;
							private _isAgent = !(isNull (agent (teamMember _x)));
							private _primaryWeapon = primaryWeapon _unit;
							private _handgunWeapon = handgunWeapon _unit;
							private _secondaryWeapon = secondaryWeapon _unit;
							
							if ((count ([_primaryWeapon, _handgunWeapon, _secondaryWeapon] select {_x isNotEqualTo "";})) isNotEqualTo 1) then {
								isNil {
									if (_unit getVariable ["KH_var_aiIsMelee", false]) then {
										_unit setVariable ["KH_var_aiIsMelee", false, true];
										private _aiFeatures = _unit getVariable ["KH_var_aiPreMeleeFeatures", []];
										private _aiSkill = _unit getVariable ["KH_var_aiPreMeleeSkill", []];

										{
											_unit enableAIFeature [_x, _aiFeatures param [_forEachIndex, true]];
										} forEach ["AUTOTARGET", "AIMINGERROR", "TARGET", "WEAPONAIM", "FSM", "AUTOCOMBAT", "COVER", "SUPPRESSION", "FIREWEAPON", "RADIOPROTOCOL"];

										{
											_unit setSkill [_x, _aiSkill param [_forEachIndex, 1]];
										} forEach ["aimingAccuracy", "aimingShake", "aimingSpeed", "spotDistance", "spotTime", "courage", "reloadSpeed", "commanding", "general"];

										_unit setBehaviourStrong ((_unit getVariable ["KH_var_aiPreMeleeBehaviour", "SAFE"]));
										_unit setUnitCombatMode ((_unit getVariable ["KH_var_aiPreMeleeCombatMode", "YELLOW"]));
										_unit setUnitPos ((_unit getVariable ["KH_var_aiPreMeleeStance", "UP"]));
										_unit doFollow (leader (group _unit));
										_unit action ["SwitchWeapon", _unit, _unit, 299];
									};
								};

								if KH_var_allowRangedMelee then {
									private _closestTarget = _unit findNearestEnemy (getPosATL _unit);

									if !(isNull _closestTarget) then {
										if ((_unit distance _closestTarget) <= KH_var_meleeRangedAiEngageDistance) then {
											_unit lookAt _closestTarget;

											if ((_unit distance _closestTarget) <= (selectRandom [3, 2.5, 2])) then {
												if _isAgent then {
													_unit setDestination [getPosATL _unit, "DoNotPlan", true];
												}
												else {
													doStop _unit;
												};
												
												if ((abs ((((_unit getRelDir _closestTarget) + 180) % 360) - 180)) <= (selectRandom [22.5, 11.25])) then {
													private _moves = getText ((configOf _closestTarget) >> "moves");
													private _moveAction = getText (configFile >> _moves >> "states" >> (animationState _closestTarget) >> "kh_meleeMainAction");
													private _gestureAction = getText (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _closestTarget) >> "kh_meleeMainAction");
													private _finalAction = [_gestureAction, _moveAction] select (_moveAction isNotEqualTo "");

													isNil {
														if (_finalAction isNotEqualTo "") then {
															switch _finalAction do {
																case "attack": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.75, "KICK", 0.25, "TACKLE", 0.25, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.25]] call KH_fnc_updateMeleeState;
																};

																case "dodge": {
																	[_unit, selectRandomWeighted ["ATTACK", 1, "TACKLE", 0.5, "STOP", 0.25]] call KH_fnc_updateMeleeState;
																};

																case "kick": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.25, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																};

																case "tackle": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.5, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																};
															};
														}
														else {
															[_unit, selectRandomWeighted ["ATTACK", 1, "KICK", 0.5, "TACKLE", 0.3, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.5]] call KH_fnc_updateMeleeState;
														};
													};
												};
											}
											else {
												if _isAgent then {
													_unit setDestination [getPosATL _closestTarget, "LEADER PLANNED", true];
												}
												else {
													_unit doMove (getPosATL _closestTarget);
												};
											};
										};
									};
								};
							}
							else {
								private _meleeWeapon = if ((getText (configFile >> "CfgWeapons" >> _primaryWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
									((_unit weaponsInfo [_primaryWeapon, false]) param [0, []]) param [0, -1];
								}
								else {
									if ((getText (configFile >> "CfgWeapons" >> _handgunWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
										((_unit weaponsInfo [_handgunWeapon, false]) param [0, []]) param [0, -1];
									}
									else {
										if ((getText (configFile >> "CfgWeapons" >> _secondaryWeapon >> "kh_meleeActions")) isNotEqualTo "") then {
											((_unit weaponsInfo [_secondaryWeapon, false]) param [0, []]) param [0, -1];
										}
										else {
											-1;
										};
									};
								};

								if (_meleeWeapon isNotEqualTo -1) then {
									isNil {
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
													_unit checkAIFeature "FIREWEAPON",
													_unit checkAIFeature "RADIOPROTOCOL"
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
											_unit enableAIFeature ["RADIOPROTOCOL", false];
											_unit setSkill ["aimingAccuracy", 0];
											_unit setSkill ["aimingShake", 0];
											_unit setSkill ["aimingSpeed", 1];
											_unit setSkill ["spotDistance", 1];
											_unit setSkill ["spotTime", 1];
											_unit setSkill ["courage", 1];
											_unit setSkill ["reloadSpeed", 0];
											_unit setSkill ["commanding", 1];
											_unit setSkill ["general", 1];
											_unit setBehaviourStrong "CARELESS";
											_unit setUnitCombatMode "BLUE";
											_unit setUnitPos "UP";
											_unit action ["SwitchWeapon", _unit, _unit, _meleeWeapon];
										}
										else {
											if ((getNumber (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> (animationState _unit) >> "kh_melee")) isNotEqualTo 1) then {
												_unit action ["SwitchWeapon", _unit, _unit, _meleeWeapon];
											};	
										};
									};

									private _closestTarget = _unit findNearestEnemy (getPosATL _unit);

									if !(isNull _closestTarget) then {
										if ((_unit distance _closestTarget) <= KH_var_meleeMeleeAiEngageDistance) then {
											_unit lookAt _closestTarget;

											if ((_unit distance _closestTarget) <= (selectRandom [3, 2.5, 2])) then {
												if _isAgent then {
													_unit setDestination [getPosATL _unit, "DoNotPlan", true];
												}
												else {
													doStop _unit;
												};
												
												if ((abs ((((_unit getRelDir _closestTarget) + 180) % 360) - 180)) <= (selectRandom [22.5, 11.25])) then {
													if ((random 1) <= 0.2) then {
														isNil {
															[_unit, "CYCLE_ATTACK_MODE"] call KH_fnc_updateMeleeState;
														};
													};

													private _moves = getText ((configOf _closestTarget) >> "moves");
													private _moveAction = getText (configFile >> _moves >> "states" >> (animationState _closestTarget) >> "kh_meleeMainAction");
													private _gestureAction = getText (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _closestTarget) >> "kh_meleeMainAction");
													private _finalAction = [_gestureAction, _moveAction] select (_moveAction isNotEqualTo "");

													isNil {
														if (_finalAction isNotEqualTo "") then {
															switch _finalAction do {
																case "attack": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.75, "PARRY", 1, "BLOCK_IN", 1, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.25]] call KH_fnc_updateMeleeState;
																};

																case "block": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.25, "KICK", 1, "TACKLE", 1]] call KH_fnc_updateMeleeState;
																};
															
																case "parry": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.25, "KICK", 1, "TACKLE", 1, "STOP", 0.25]] call KH_fnc_updateMeleeState;
																};

																case "dodge": {
																	[_unit, selectRandomWeighted ["ATTACK", 1, "TACKLE", 0.5, "STOP", 0.25]] call KH_fnc_updateMeleeState;
																};

																case "kick": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.25, "BLOCK_IN", 0.5, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																};

																case "tackle": {
																	[_unit, selectRandomWeighted ["ATTACK", 0.5, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																};
															};
														}
														else {
															[_unit, selectRandomWeighted ["ATTACK", 1, "PARRY", 0.75, "KICK", 0.5, "TACKLE", 0.3, "BLOCK_IN", 0.75, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.5]] call KH_fnc_updateMeleeState;
														};
													};
												};
											}
											else {
												if _isAgent then {
													_unit setDestination [getPosATL _closestTarget, "LEADER PLANNED", true];
												}
												else {
													_unit doMove (getPosATL _closestTarget);
												};
											};
										};
									};
								}
								else {
									isNil {
										if (_unit getVariable ["KH_var_aiIsMelee", false]) then {
											_unit setVariable ["KH_var_aiIsMelee", false, true];
											private _aiFeatures = _unit getVariable ["KH_var_aiPreMeleeFeatures", []];
											private _aiSkill = _unit getVariable ["KH_var_aiPreMeleeSkill", []];

											{
												_unit enableAIFeature [_x, _aiFeatures param [_forEachIndex, true]];
											} forEach ["AUTOTARGET", "AIMINGERROR", "TARGET", "WEAPONAIM", "FSM", "AUTOCOMBAT", "COVER", "SUPPRESSION", "FIREWEAPON", "RADIOPROTOCOL"];

											{
												_unit setSkill [_x, _aiSkill param [_forEachIndex, 1]];
											} forEach ["aimingAccuracy", "aimingShake", "aimingSpeed", "spotDistance", "spotTime", "courage", "reloadSpeed", "commanding", "general"];

											_unit setBehaviourStrong ((_unit getVariable ["KH_var_aiPreMeleeBehaviour", "SAFE"]));
											_unit setUnitCombatMode ((_unit getVariable ["KH_var_aiPreMeleeCombatMode", "YELLOW"]));
											_unit setUnitPos ((_unit getVariable ["KH_var_aiPreMeleeStance", "UP"]));
											_unit action ["SwitchWeapon", _unit, _unit, 299];
										};
									};

									if KH_var_allowRangedMelee then {
										private _closestTarget = _unit findNearestEnemy (getPosATL _unit);

										if !(isNull _closestTarget) then {
											if ((_unit distance _closestTarget) <= KH_var_meleeRangedAiEngageDistance) then {
												_unit lookAt _closestTarget;

												if ((_unit distance _closestTarget) <= (selectRandom [3, 2.5, 2])) then {
													if _isAgent then {
														_unit setDestination [getPosATL _unit, "DoNotPlan", true];
													}
													else {
														doStop _unit;
													};
													
													if ((abs ((((_unit getRelDir _closestTarget) + 180) % 360) - 180)) <= (selectRandom [22.5, 11.25])) then {
														private _moves = getText ((configOf _closestTarget) >> "moves");
														private _moveAction = getText (configFile >> _moves >> "states" >> (animationState _closestTarget) >> "kh_meleeMainAction");
														private _gestureAction = getText (configFile >> (getText (configFile >> _moves >> "gestures")) >> "states" >> (gestureState _closestTarget) >> "kh_meleeMainAction");
														private _finalAction = [_gestureAction, _moveAction] select (_moveAction isNotEqualTo "");

														isNil {
															if (_finalAction isNotEqualTo "") then {
																switch _finalAction do {
																	case "attack": {
																		[_unit, selectRandomWeighted ["ATTACK", 0.75, "KICK", 0.25, "TACKLE", 0.25, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.25]] call KH_fnc_updateMeleeState;
																	};

																	case "dodge": {
																		[_unit, selectRandomWeighted ["ATTACK", 1, "TACKLE", 0.5, "STOP", 0.25]] call KH_fnc_updateMeleeState;
																	};

																	case "kick": {
																		[_unit, selectRandomWeighted ["ATTACK", 0.25, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																	};

																	case "tackle": {
																		[_unit, selectRandomWeighted ["ATTACK", 0.5, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 1]] call KH_fnc_updateMeleeState;
																	};
																};
															}
															else {
																[_unit, selectRandomWeighted ["ATTACK", 1, "KICK", 0.5, "TACKLE", 0.3, selectRandom [["DODGE", "LEFTWARD"], ["DODGE", "RIGHTWARD"], ["DODGE", "BACKWARD"], ["DODGE", "BACKWARD_RIGHTWARD"], ["DODGE", "BACKWARD_LEFTWARD"]], 0.5]] call KH_fnc_updateMeleeState;
															};
														};
													};
												}
												else {
													if _isAgent then {
														_unit setDestination [getPosATL _closestTarget, "LEADER PLANNED", true];
													}
													else {
														_unit doMove (getPosATL _closestTarget);
													};
												};
											};
										};
									};
								};
							};
						} forEach ((KH_var_allLocalEntities - [KH_var_playerUnit]) select {_x isKindOf "Man";});
					};
				};
			};
		},
		true,
		"-1",
		false
	] call KH_fnc_execute;
};

nil;