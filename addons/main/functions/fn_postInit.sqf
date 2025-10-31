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

		[
			"AllVehicles",
			"init", 
			{
				params ["_unit"];
				
				if !(_unit isKindOf "Man") then {
					_unit addAction [
						"Unload Incapacitated",
						{
							params ["_target"];

							{
								if (((lifeState _x) isEqualTo "INCAPACITATED") || ((lifeState _x) isEqualTo "UNCONSCIOUS")) then {
									moveOut _x;
								};
							} forEach (crew _target);
						},
						[],
						1.5,
						false,
						true,
						"",
						"((((call KH_fnc_getViewTarget) select 1) < 2) && ((crew _target) isNotEqualTo []))",
						50,
						false,
						"",
						""
					];
				};
			},
			true,
			[],
			true
		] call CBA_fnc_addClassEventHandler;
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

	if isServer then {
		if !(isNil "KH_var_missionStartSuspension") then {
			{
				private KH_var_missionStartSuspensionEntities = [];
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
								KH_var_suspensionDisplay = ["KH_ResourceKHControl", "PLAYERS LOADING...", [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], [0, 0, 0]] call KH_fnc_draw2d;
							}
							else {
								if (isNull KH_var_suspensionDisplay) then {
									KH_var_suspensionDisplay = ["KH_ResourceKHControl", "PLAYERS LOADING...", [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], [0, 0, 0]] call KH_fnc_draw2d;
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
										KH_var_suspensionDisplay closeDisplay 2;
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

		if KH_var_khMedical then {
			[
				"CAManBase",
				"init", 
				{
					params ["_unit"];
					
					if (_unit getVariable ["KH_var_allowMedicalHandling", true]) then {
						[
							["ENTITY", _unit, "PERSISTENT"],
							"HandleDamage",
							[],
							{
								params ["_unit", "_selection", "_damage", "_source", "_projectile", "_hitPartIndex", "_instigator", "_hitPoint", "_directHit", "_context"];
								_hitPoint = toLowerANSI _hitPoint;

								if ((_context isEqualTo 3) && (_hitPoint isEqualTo "hithead")) exitWith {
									_unit getHitPointDamage "hithead";
								};

								if ((_context isEqualTo 0) && ((abs (_damage - (_unit getHitPointDamage _hitPoint) - 1)) < 0.001) && (_projectile isEqualTo "") && (isNull _source) && (isNull _instigator)) exitWith {
									_damage;
								};

								private _totalDamage = false;

								private _currentDamage = if (((_context isEqualTo 0) || (_context isEqualTo 4))) then {
									_totalDamage = true;
									damage _unit;
								}
								else {
									(_unit getHitPointDamage _hitPoint);
								};

								if ((_projectile isEqualTo "") && !(_unit getVariable ["KH_var_allowImpactDamage", true]) && _totalDamage) exitWith {
									_currentDamage;
								};

								private _hitPointType = switch true do {
									case (("head" in _hitPoint) || ("face" in _hitPoint)): {
										"head";
									};

									case ("neck" in _hitPoint): {
										"neck";
									};

									case (("arm" in _hitPoint) || ("shoulder" in _hitPoint) || ("clavicle" in _hitPoint) || ("hand" in _hitPoint)): {
										"arm";
									};

									case (("leg" in _hitPoint) || ("calf" in _hitPoint) || ("thigh" in _hitPoint)): {
										"leg";
									};

									case (("chest" in _hitPoint) || ("spine" in _hitPoint) || ("torso" in _hitPoint) || ("diaphragm" in _hitPoint)): {
										"torso";
									};

									case (("pelvis" in _hitPoint) || ("hip" in _hitPoint) || ("abdomen" in _hitPoint)): {
										"pelvis";
									};
									
									default {
										"";
									};
								};

								private _incapacitated = (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS"));
								private _withstanding = _unit getVariable ["KH_var_withstanding", false];
								_unit setVariable ["KH_var_currentProcessedDamage", _damage];

								{
									private _currentProcessedDamage = _unit getVariable "KH_var_currentProcessedDamage";
									_unit setVariable ["KH_var_currentProcessedDamage", call _x];
								} forEach (_unit getVariable ["KH_var_allDamageHandlers", []]);
								
								private _allowAbsoluteMultipliers = _unit getVariable ["KH_var_allowAbsoluteDamageMultipliers", true];
								private "_absoluteHitPointDamageMultiplier";
								private "_hitPointDamageMultiplier";
								private "_absoluteDamageMultiplier";

								switch _hitPointType do {
									case "head": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierHead] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierHead", 1];
									};

									case "neck": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierNeck] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierNeck", 1];
									};

									case "arm": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierArm] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierArm", 1];
									};

									case "leg": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierLeg] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierLeg", 1];
									};

									case "torso": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierTorso] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierTorso", 1];
									};

									case "pelvis": {
										_absoluteHitPointDamageMultiplier = [1, KH_var_absoluteDamageMultiplierPelvis] select _allowAbsoluteMultipliers;
										_hitPointDamageMultiplier = _unit getVariable ["KH_var_damageMultiplierPelvis", 1];
									};
									
									default {
										_absoluteHitPointDamageMultiplier = 1;
										_hitPointDamageMultiplier = 1;
									};
								};

								_absoluteDamageMultiplier = [1, KH_var_absoluteDamageMultiplier] select _allowAbsoluteMultipliers;

								private _finalDamage = (
									(
										(_currentDamage + (_unit getVariable "KH_var_currentProcessedDamage")) * 
										_hitPointDamageMultiplier * 
										_absoluteHitPointDamageMultiplier * 
										(_unit getVariable ["KH_var_damageMultiplier", 1]) * 
										_absoluteDamageMultiplier
									) min 1
								) max 0;

								private _incapacitationThreshold = _unit getVariable ["KH_var_incapacitationThreshold", KH_var_incapacitationThreshold];

								if _totalDamage then {
									private _absoluteTotalDamageMultiplier = [1, KH_var_absoluteTotalDamageMultiplier] select _allowAbsoluteMultipliers;
									private _totalDamageMultiplier = _unit getVariable ["KH_var_totalDamageMultiplier", 1];

									_finalDamage = if (_finalDamage >= _incapacitationThreshold) then {
										if _withstanding then {
											(
												_finalDamage * 
												(_unit getVariable ["KH_var_withstandingDamageMultiplier", 1]) * 
												([1, KH_var_absoluteWithstandingDamageMultiplier] select _allowAbsoluteMultipliers) *
												(_unit getVariable ["KH_var_totalDamageMultiplier", 1]) * 
												_absoluteTotalDamageMultiplier
											) min (_unit getVariable ["KH_var_maximumTotalDamage", 1]);
										}
										else {
											if _incapacitated then {
												(
													_finalDamage * 
													(_unit getVariable ["KH_var_incapacitatedDamageMultiplier", 1]) * 
													([1, KH_var_absoluteIncapacitatedDamageMultiplier] select _allowAbsoluteMultipliers) * 
													(_unit getVariable ["KH_var_totalDamageMultiplier", 1]) *
													_absoluteTotalDamageMultiplier
												) min (_unit getVariable ["KH_var_maximumTotalDamage", 1]);
											}
											else {
												if (KH_var_allowIncapacitation && (isPlayer _unit) || (_unit getVariable ["KH_var_allowAiIncapacitation", KH_var_allowAiIncapacitation])) then {
													_unit setUnconscious true;

													[
														[_unit, time + (_unit getVariable ["KH_var_incapacitationDuration", KH_var_incapacitationDuration])],
														{
															params ["_unit", "_maximumTime"];

															if (time >= _maximumTime) then {
																if (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState player) isEqualTo "UNCONSCIOUS")) then {
																	_unit setDamage 1;
																};

																[_handlerId] call KH_fnc_removeHandler;
															}
															else {
																if !(((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState player) isEqualTo "UNCONSCIOUS")) then {
																	[_handlerId] call KH_fnc_removeHandler;
																};
															};
														},
														true,
														0,
														false
													] call KH_fnc_execute;

													if ((_unit isEqualTo player) && (_unit getVariable ["KH_var_withstandingAllowed", KH_var_withstandingAllowed])) then {
														[
															[true, false],
															[
																"<img image='\A3\ui_f\data\igui\cfg\actions\heal_ca.paa' size='1.8'/><br/>WITHSTAND",
																nil,
																nil,
																"<img image='\A3\ui_f\data\igui\cfg\actions\heal_ca.paa' size='1.8'/><br/>WITHSTANDING..."
															],
															[],
															{
																player setUnconscious false;
																player setVariable ["KH_var_withstanding", true, true];
																[player, "KH_var_withstanding", false, true, true] call KH_fnc_setRespawnVariable;
															},
															[
																{
																	(((lifeState player) isEqualTo "INCAPACITATED") || ((lifeState player) isEqualTo "UNCONSCIOUS"));
																}
															],
															false,
															true,
															true,
															[_unit getVariable ["KH_var_withstandActionDuration", KH_var_withstandActionDuration], "BAR"],
															2,
															true,
															true,
															false,
															false,
															true,
															"",
															"",
															[true, false]
														] call KH_fnc_addAction;
													};

													[
														[_unit, false],
														[
															["<img image='\A3\ui_f\data\igui\cfg\actions\heal_ca.paa' size='1.8'/><br/>REVIVE ", name _unit] joinString "",
															nil,
															nil,
															["<img image='\A3\ui_f\data\igui\cfg\actions\heal_ca.paa' size='1.8'/><br/>STOP REVIVING ", name _unit] joinString ""
														],
														[_unit],
														[
															{
																params ["_unit"];
																_caller playActionNow (["MedicOther", "Medic"] select (_caller isEqualTo _unit));
															},
															{},
															{
																_caller playActionNow "MedicStop";
															},
															{
																_caller playActionNow "MedicStop";
															},
															{
																params ["_unit"];
																private _newDamage = 1 - (abs ((_caller getVariable ["KH_var_maximumHeal", KH_var_maximumHeal]) - 1));

																if ((damage _unit) > _newDamage) then {
																	_unit setDamage _newDamage;
																};

																private _damages = getAllHitPointsDamage _unit;

																{
																	if (((_damages select 2) select _forEachIndex) > _newDamage) then {
																		_unit setHitPointDamage [_x, _newDamage];
																	};
																} forEach (_damages select 0);
																
																_unit setUnconscious false;

																if (_unit getVariable ["KH_var_withstanding", false]) then {
																	_unit setVariable ["KH_var_withstanding", false, true];
																};

																if !("Medikit" in (items _caller)) then {
																	for "_i" from 1 to KH_var_requiredReviveFirstAidKits do {
																		_caller removeItem "FirstAidKit";
																	};
																};
															},
															{}
														],
														[
															{
																params ["_unit"];

																if (_caller isEqualTo _unit) then {
																	(((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS"));
																}
																else {
																	(
																		(((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS")) &&
																		(((lifeState _caller) isNotEqualTo "INCAPACITATED") && ((lifeState _caller) isNotEqualTo "UNCONSCIOUS"))
																	);
																};
															},
															{
																params ["_unit"];

																if (_caller isEqualTo _unit) then {
																	if (
																		(_unit getVariable ["KH_var_selfRevive", KH_var_selfRevive]) &&
																		(!KH_var_requiredSelfReviveMedikit || ("Medikit" in (items _caller))) 
																	   ) then {
																		_duration = (
																			(_unit getVariable ["KH_var_selfReviveDuration", KH_var_selfReviveDuration]) * 
																			(_unit getVariable ["KH_var_selfReviveDurationMultiplier", 1])
																		);
																		
																		(
																			(!KH_var_requiredSelfReviveMedikit || (([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_requiredReviveFirstAidKits)) &&
																			(!KH_var_medicOnlySelfRevive || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean))
																		);
																	}
																	else {
																		false;
																	};
																}
																else {
																	_duration = (
																		(_unit getVariable ["KH_var_reviveDuration", KH_var_reviveDuration]) * 
																		(_unit getVariable ["KH_var_reviveDurationMultiplier", 1])
																	);

																	if KH_var_requiredReviveMedikit then {
																		(
																			(!KH_var_medicOnlyRevive || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) && 
																			("Medikit" in (items _caller))
																		);
																	}
																	else {
																		(
																			(!KH_var_medicOnlyRevive || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) &&
																			(
																				(([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_requiredReviveFirstAidKits) ||
																				("Medikit" in (items _caller))
																			)
																		);
																	};
																};
															},
															{},
															{},
															{}
														],
														false,
														true,
														"PLAYERS",
														[_unit getVariable ["KH_var_reviveDuration", KH_var_reviveDuration], "BAR"],
														2,
														true,
														true,
														false,
														false,
														true,
														"",
														"",
														[false, false]
													] call KH_fnc_addAction;

													[
														[_unit, false],
														[
															"Load To Nearest Vehicle",
															nil,
															nil,
															"Loading..."
														],
														[_unit],
														{
															_target moveInAny (((_target nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) select 0);
														},
														[
															{
																params ["_unit"];
																(((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS"));
															},
															{
																params ["_unit"];
																(((_unit nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) isNotEqualTo []);
															},
															{},
															{},
															{}
														],
														false,
														true,
														"PLAYERS",
														[3, "BAR"],
														2,
														true,
														true,
														false,
														false,
														true,
														"",
														"",
														[false, false]
													] call KH_fnc_addAction;											
												};
												
												private _incapacitatedDamageMultiplier getVariable ["KH_var_incapacitatedDamageMultiplier", 1];

												(
													(
														((_finalDamage - _incapacitationThreshold) * _incapacitatedDamageMultiplier * ([1, KH_var_absoluteIncapacitatedDamageMultiplier] select _allowAbsoluteMultipliers)) +
														(_incapacitationThreshold + _incapacitatedDamage) *
														(_unit getVariable ["KH_var_totalDamageMultiplier", 1]) *
														_absoluteTotalDamageMultiplier
													) min (_unit getVariable ["KH_var_maximumTotalDamage", 1])
												) max 0;
											};
										};
									}
									else {
										(_finalDamage * (_unit getVariable ["KH_var_totalDamageMultiplier", 1]) * _absoluteTotalDamageMultiplier) min (_unit getVariable ["KH_var_maximumTotalDamage", 1]);
									};
								};

								_finalDamage; 
							}
						] call KH_fnc_addEventHandler;
					};
				},
				true,
				[],
				true
			] call CBA_fnc_addClassEventHandler;
		};
	};
};

nil;