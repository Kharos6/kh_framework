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
						
						(((count KH_var_allPlayerUnits) >= _initialPlayerCount)) || (CBA_missionTime > 60);
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
			KH_var_classDamageMultipliers = createHashMapFromArray (parseSimpleArray (["[", KH_var_classDamageMultipliersRaw, "]"] joinString ""));
			KH_var_childClassDamageMultipliers = parseSimpleArray (["[", KH_var_childClassDamageMultipliersRaw, "]"] joinString "");
			
			[
				"CAManBase",
				"init", 
				{
					params ["_unit"];
					if !(isNil {_unit getVariable "KH_var_initComplete";}) exitWith {};
					private _classDamageMultiplier = KH_var_classDamageMultipliers get (typeOf _unit);
					private "_childClassDamageMultiplier";
					
					{
						_x params ["_class", "_multiplier"];
						
						if (_unit isKindOf _class) then {
							_childClassDamageMultiplier = _multiplier;
							break;
						};
					} forEach KH_var_childClassDamageMultipliers;
					
					if !(isNil "_classDamageMultiplier") then {
						_unit setVariable ["KH_var_classDamageMultiplier", _classDamageMultiplier, true];
					};

					if !(isNil "_childClassDamageMultiplier") then {
						_unit setVariable ["KH_var_childClassDamageMultiplier", _childClassDamageMultiplier, true];
					};

					_unit setVariable ["KH_var_initComplete", true, true];
					
					if (_unit getVariable ["KH_var_khMedicalHandling", true]) then {
						[_unit, "KH_var_stabilized", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_withstanding", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_incapacitated", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_beingRevived", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_beingStabilized", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_beingTreated", false, true, true] call KH_fnc_setRespawnVariable;
						[_unit, "KH_var_isTreating", false, true, true] call KH_fnc_setRespawnVariable;

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

								private _currentDamage = if ((_context isEqualTo 0) || (_context isEqualTo 4)) then {
									_totalDamage = true;
									damage _unit;
								}
								else {
									_unit getHitPointDamage _hitPoint;
								};

								if (_projectile isEqualTo "") then {
									_damage = _damage * KH_var_absoluteImpactDamageMultiplier;
								};

								if (_damage isEqualTo 0) exitWith {
									_currentDamage;
								};

								private _hitPointType = switch _hitPoint do {
									case "hithead";
									case "hitface": {
										"head";
									};

									case "hitneck": {
										"neck";
									};

									case "hitpelvis";
									case "hitabdomen": {
										"pelvis";
									};

									case "hitdiaphragm";
									case "hitchest": {
										"torso";	
									};

									case "hithands";
									case "hitarms": {
										"arms";
									};

									case "hitlegs": {
										"legs";
									};
									
									default {
										"body";
									};
								};
								
								private _isPlayer = isPlayer _unit;
								private _incapacitated = _unit getVariable ["KH_var_incapacitated", false];
								private _withstanding = _unit getVariable ["KH_var_withstanding", false];
								_unit setVariable ["KH_var_currentProcessedDamage", _damage];

								{
									private _currentProcessedDamage = _unit getVariable "KH_var_currentProcessedDamage";
									_unit setVariable ["KH_var_currentProcessedDamage", call _x];
								} forEach (_unit getVariable ["KH_var_allDamageHandlers", []]);

								private _processedDamage = (
									((_unit getVariable "KH_var_currentProcessedDamage") max 0) * 
									(
										switch _hitPointType do {
											case "head": {
												KH_var_absoluteDamageMultiplierHead;
											};

											case "neck": {
												KH_var_absoluteDamageMultiplierNeck;
											};

											case "arm": {
												KH_var_absoluteDamageMultiplierArm;
											};

											case "leg": {
												KH_var_absoluteDamageMultiplierLeg;
											};

											case "torso": {
												KH_var_absoluteDamageMultiplierTorso;
											};

											case "pelvis": {
												KH_var_absoluteDamageMultiplierPelvis;
											};
											
											default {
												1;
											};
										}
									) *
									([KH_var_absoluteAiDamageMultiplier, KH_var_absolutePlayerDamageMultiplier] select _isPlayer) * 
									KH_var_absoluteDamageMultiplier *
									(_unit getVariable ["KH_var_classDamageMultiplier", 1]) * 
									(_unit getVariable ["KH_var_childClassDamageMultiplier", 1]) *
									(_unit getVariable ["KH_var_damageMultiplier", 1])
								);

								if _totalDamage then {
									if (
										((_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) >= KH_var_incapacitationThreshold) && 
										(((_unit getVariable ["KH_var_allowIncapacitation", KH_var_allowIncapacitation]) && _isPlayer) || (_unit getVariable ["KH_var_allowIncapacitation", KH_var_allowAiIncapacitation]))
									   ) then {
										if _withstanding then {
											(
												_currentDamage + 
												(
													_processedDamage * 
													KH_var_absoluteWithstandingDamageMultiplier *
													KH_var_absoluteTotalDamageMultiplier
												)
											) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
										}
										else {
											if _incapacitated then {
												(
													_currentDamage + 
													(
														_processedDamage * 
														KH_var_absoluteIncapacitatedDamageMultiplier *
														KH_var_absoluteTotalDamageMultiplier
													)
												) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
											}
											else {
												_unit setUnconscious true;
												_unit setVariable ["KH_var_incapacitated", true, true];

												if KH_var_incapacitatedCaptives then {
													_unit setCaptive true;
												};

												if (_unit isEqualTo player) then {
													KH_var_incapacitationFade = ppEffectCreate ["ColorCorrections", 892003];
													KH_var_incapacitationFade ppEffectEnable true;
													KH_var_incapacitationFade ppEffectAdjust [1, 1, 0, [0, 0, 0, 0], [1, 1, 1, 0], [0.33, 0.33, 0.33, 0], [0, 0, 0, 0, 0, 0, 4]];
													KH_var_incapacitationFade ppEffectCommit KH_var_incapacitationDeathTime;
												};

												[
													[_unit, diag_tickTime + KH_var_incapacitationDeathTime],
													{
														params ["_unit", "_maximumTime"];

														if (diag_tickTime >= _maximumTime) then {
															if (_unit getVariable ["KH_var_incapacitated", false]) then {
																_unit setDamage 1;
															};

															[_handlerId] call KH_fnc_removeHandler;
														}
														else {
															if !(_unit getVariable ["KH_var_incapacitated", false]) then {
																[_handlerId] call KH_fnc_removeHandler;
															};
														};
													},
													true,
													0,
													false
												] call KH_fnc_execute;

												if (_unit isEqualTo player) then {
													if KH_var_withstandingAllowed then {
														[
															[],
															{
																[
																	[true, false],
																	[
																		"<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>WITHSTAND",
																		nil,
																		nil,
																		"<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>WITHSTANDING"
																	],
																	[],
																	{
																		player setUnconscious false;

																		if KH_var_incapacitatedCaptives then {
																			player setCaptive false;
																		};

																		player setVariable ["KH_var_withstanding", true, true];
																		KH_var_withstandingEffectRadial = ppEffectCreate ["radialBlur", 892000];
																		KH_var_withstandingEffectRadial ppEffectEnable true;
																		KH_var_withstandingEffectRadial ppEffectAdjust [0.01, 0.01, 0.33, 0.33];
																		KH_var_withstandingEffectRadial ppEffectCommit 1;
																		KH_var_withstandingEffectChromatic = ppEffectCreate ["ChromAberration", 892001];
																		KH_var_withstandingEffectChromatic ppEffectEnable true;
																		KH_var_withstandingEffectChromatic ppEffectAdjust [0.005, 0.005, false];
																		KH_var_withstandingEffectChromatic ppEffectCommit 1;
																		KH_var_withstandingEffectWet = ppEffectCreate ["WetDistortion", 892002];
																		KH_var_withstandingEffectWet ppEffectEnable true;
																		KH_var_withstandingEffectWet ppEffectAdjust [1, 0.015, 0.015, 1, 1, 1, 1, 0.05, 0.01, 0.05, 0.01, 0.1, 0.1, 0.2, 0.2];
																		KH_var_withstandingEffectWet ppEffectCommit 1;

																		if (KH_var_withstandDuration isNotEqualTo 0) then {
																			[
																				[diag_tickTime + KH_var_withstandDuration],
																				{
																					params ["_deadline"];

																					if ((diag_tickTime >= _deadline) || !(alive player) || !(player getVariable ["KH_var_incapacitated", false])) then {
																						if (player getVariable ["KH_var_incapacitated", false]) then {
																							player setUnconscious true;

																							if KH_var_incapacitatedCaptives then {
																								player setCaptive true;
																							};
																						};

																						player setVariable ["KH_var_withstanding", false, true];
																						[_hanlderId] call KH_fnc_removeHandler;
																					};
																				},
																				true,
																				0,
																				false
																			] call KH_fnc_execute;
																		};

																		if KH_var_withstandingAllowCancel then {
																			[
																				[true, false],
																				"<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>STOP WITHSTANDING",
																				[],
																				{
																					player setUnconscious true;

																					if KH_var_incapacitatedCaptives then {
																						player setCaptive true;
																					};
																					
																					player setVariable ["KH_var_withstanding", false, true];
																				},
																				[
																					{
																						((player getVariable ["KH_var_withstanding", false]) && (player getVariable ["KH_var_incapacitated", false]));
																					}
																				],
																				false,
																				true,
																				true,
																				[KH_var_withstandActionDuration, KH_var_withstandProgressDisplay],
																				2,
																				false,
																				true,
																				false,
																				false,
																				true,
																				"",
																				"",
																				[true, false]
																			] call KH_fnc_addAction;
																		};
																	},
																	[
																		{
																			((player getVariable ["KH_var_incapacitated", false]) && (alive player));
																		},
																		{
																			(!KH_var_withstandingRequireStabilization || (player getVariable ["KH_var_stabilized", false]));
																		}
																	],
																	false,
																	true,
																	true,
																	[KH_var_withstandActionDuration, KH_var_withstandProgressDisplay],
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
															},
															true,
															str KH_var_withstandDelay,
															false
														] call KH_fnc_execute;
													};

													if KH_var_allowGivingUp then {
														[
															[true, false],
															[
																"<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_forceRespawn_ca.paa' size='2.4'/><br/>GIVE UP",
																nil,
																nil,
																"<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_forceRespawn_ca.paa' size='2.4'/><br/>GIVING UP"
															],
															[],
															{
																player setDamage 1;
															},
															[
																{
																	((player getVariable ["KH_var_incapacitated", false]) && (alive player));
																}
															],
															false,
															true,
															true,
															[3, KH_var_giveUpProgressDisplay],
															2,
															false,
															true,
															false,
															false,
															true,
															"",
															"",
															[true, false]
														] call KH_fnc_addAction;
													};
												};

												[
													[_unit, false],
													[
														["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_reviveMedic_ca.paa' size='2.4'/><br/>REVIVE ", toUpper (name _unit)] joinString "",
														nil,
														nil,
														["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_reviveMedic_ca.paa' size='2.4'/><br/>REVIVING ", toUpper (name _unit)] joinString ""
													],
													[_unit],
													[
														{
															_target setVariable ["KH_var_beingRevived", true, true];

															if (isNull (objectParent _caller)) then {
																if (_caller isEqualTo _target) then {
																	if (!("medic" in (animationState _caller)) && (_caller getVariable ["KH_var_withstanding", false])) then {
																		_caller playActionNow "Medic";
																	};
																}
																else {
																	if !("medic" in (animationState _caller)) then {
																		_caller playActionNow "MedicOther";
																	};
																};
															};

															_duration = [
																[KH_var_reviveDuration, KH_var_selfReviveDuration] select (_caller isEqualTo _target), 
																[KH_var_reviveMedicDuration, KH_var_selfReviveMedicDuration] select (_caller isEqualTo _target)
															] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean);

															nil;
														},
														{
															if (isNull (objectParent _caller)) then {
																if (_caller isEqualTo _target) then {
																	if (!("medic" in (animationState _caller)) && (_caller getVariable ["KH_var_withstanding", false])) then {
																		_caller playActionNow "Medic";
																	};
																}
																else {
																	if !("medic" in (animationState _caller)) then {
																		_caller playActionNow "MedicOther";
																	};
																};
															};
														},
														{
															_target setVariable ["KH_var_beingRevived", false, true];
														},
														{
															_target setVariable ["KH_var_beingRevived", false, true];
														},
														{
															private _damageOffset = [
																[KH_var_reviveHeal, KH_var_selfReviveHeal] select (_caller isEqualTo _target),
																[KH_var_reviveHealMedic, KH_var_selfReviveHealMedic] select (_caller isEqualTo _target)
															] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean);

															_target setDamage (([(damage _target) - _damageOffset, 0] select (((damage _target) - _damageOffset) <= 0.25)) max 0);
															private _damages = getAllHitPointsDamage _target;

															{
																_target setHitPointDamage [_x, (((_damages select 2) select _forEachIndex) - _damageOffset) max 0];
															} forEach (_damages select 0);

															[
																[_target, _caller],
																{
																	params ["_target", "_caller"];
																	_target setUnconscious false;

																	if KH_var_incapacitatedCaptives then {
																		_target setCaptive false;
																	};

																	if (_target isEqualTo player) then {
																		if (player getVariable ["KH_var_withstanding", false]) then {
																			ppEffectDestroy [KH_var_withstandingEffectRadial, KH_var_withstandingEffectChromatic, KH_var_withstandingEffectWet];
																		};

																		ppEffectDestroy KH_var_incapacitationFade;
																	};
																},
																_target,
																true,
																false
															] call KH_fnc_execute;

															if !("Medikit" in (items _caller)) then {
																for "_i" from 1 to ([KH_var_reviveRequiredFirstAidKits, KH_var_selfReviveRequiredFirstAidKits] select (_caller isEqualTo _target)) do {
																	_caller removeItem "FirstAidKit";
																};
															};

															_target setVariable ["KH_var_incapacitated", false, true];
															_target setVariable ["KH_var_withstanding", false, true];
															_target setVariable ["KH_var_stabilized", false, true];
															_target setVariable ["KH_var_beingRevived", false, true];
															_target setVariable ["KH_var_beingStabilized", false, true];
														},
														{
															params ["_unit"];
															_unit setVariable ["KH_var_incapacitated", false, true];
															_unit setVariable ["KH_var_withstanding", false, true];
															_unit setVariable ["KH_var_stabilized", false, true];
															_unit setVariable ["KH_var_beingRevived", false, true];
															_unit setVariable ["KH_var_beingStabilized", false, true];
														}
													],
													[
														{
															params ["_unit"];
															((_unit getVariable ["KH_var_incapacitated", false]) && (alive _unit));
														},
														{
															if (_target getVariable ["KH_var_beingStabilized", false]) exitWith {
																false;
															};

															if (KH_var_reviveRequireStabilization && !(_target getVariable ["KH_var_stabilized", false])) exitWith {
																false;
															};

															if (_caller isEqualTo _target) then {
																if KH_var_selfRevive then {																	
																	(
																		(
																			(KH_var_selfReviveRequireMedikit && ("Medikit" in (items _caller))) ||
																			(!KH_var_selfReviveRequireMedikit && (([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_selfReviveRequiredFirstAidKits))
																		) &&
																		(!KH_var_selfReviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) &&
																		(!KH_var_selfReviveRequireWithstanding || (_caller getVariable ["KH_var_withstanding", false]))
																	);
																}
																else {
																	false;
																};
															}
															else {
																if !(_caller getVariable ["KH_var_incapacitated", false]) then {																	
																	if ((_caller getVariable ["KH_var_withstanding", false]) && !KH_var_withstandingAllowRevivingOthers) then {
																		false;
																	}
																	else {
																		if KH_var_reviveRequireMedikit then {
																			(
																				(!KH_var_reviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) && 
																				("Medikit" in (items _caller))
																			);
																		}
																		else {
																			(
																				(!KH_var_reviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) &&
																				(
																					(([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_reviveRequiredFirstAidKits) ||
																					("Medikit" in (items _caller))
																				)
																			);
																		};
																	};
																}
																else {
																	false;
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
													[KH_var_reviveDuration, KH_var_reviveProgressDisplay],
													2,
													true,
													true,
													false,
													false,
													true,
													"",
													"",
													[false, true]
												] call KH_fnc_addAction;

												[
													[_unit, false],
													[
														"<img image='\A3\ui_f\data\igui\cfg\actions\loadVehicle_ca.paa' size='1.8'/><br/>LOAD TO NEAREST VEHICLE",
														nil,
														nil,
														"<img image='\A3\ui_f\data\igui\cfg\actions\loadVehicle_ca.paa' size='1.8'/><br/>LOADING"
													],
													[_unit],
													{
														_target moveInAny (((_target nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) select 0);
													},
													[
														{
															params ["_unit"];
															(((_unit getVariable ["KH_var_incapacitated", false])) && (isNull (objectParent _unit)) && (alive _unit));
														},
														{
															((((_target nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) isNotEqualTo []) && (_target isNotEqualTo _caller));
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
													false,
													false,
													false,
													false,
													true,
													"",
													"",
													[false, true]
												] call KH_fnc_addAction;				
											
												if KH_var_stabilizationAllowed then {
													[
														[_unit, false],
														[
															["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_revive_ca.paa' size='2.4'/><br/>STABILIZE ", toUpper (name _unit)] joinString "",
															nil,
															nil,
															["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_revive_ca.paa' size='2.4'/><br/>STABILIZING ", toUpper (name _unit)] joinString ""
														],
														[_unit],
														[
															{
																_target setVariable ["KH_var_beingStabilized", true, true];

																if (isNull (objectParent _caller)) then {
																	_caller playActionNow "MedicOther";
																};

																_duration = [KH_var_stabilizationDuration, KH_var_stabilizationDurationMedic] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean);
																nil;
															},
															{
																if (isNull (objectParent _caller)) then {
																	if !("medic" in (animationState _caller)) then {
																		_caller playActionNow "MedicOther";
																	};
																};
															},
															{
																_target setVariable ["KH_var_beingStabilized", false, true];
															},
															{
																_target setVariable ["KH_var_beingStabilized", false, true];
															},
															{
																if !("Medikit" in (items _caller)) then {
																	for "_i" from 1 to KH_var_stabilizationRequiredFirstAidKits do {
																		_caller removeItem "FirstAidKit";
																	};
																};
																
																_target setVariable ["KH_var_beingStabilized", false, true];
																_target setVariable ["KH_var_stabilized", true, true];
															},
															{}
														],
														[
															{
																params ["_unit"];
																((_unit getVariable ["KH_var_incapacitated", false]) && (alive _unit));
															},
															{
																if (_target getVariable ["KH_var_beingRevived", false]) exitWith {
																	false;
																};

																if ((KH_var_stabilizationMedicOnly && !([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) || (_caller isEqualTo _target)) exitWith {
																	false;
																};

																if KH_var_stabilizationRequireMedikit then {
																	"Medikit" in (items _caller);
																}
																else {
																	([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_stabilizationRequiredFirstAidKits;
																};									
															},
															{},
															{},
															{}
														],
														false,
														true,
														"PLAYERS",
														[KH_var_stabilizationDuration, KH_var_stabilizationProgressDisplay],
														2,
														true,
														false,
														false,
														false,
														true,
														"",
														"",
														[false, true]
													] call KH_fnc_addAction;
												};

												(
													KH_var_incapacitationThreshold + (((_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) - KH_var_incapacitationThreshold) * KH_var_absoluteIncapacitatedDamageMultiplier)
												) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
											};
										};
									}
									else {
										(_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
									};
								}
								else {
									(_currentDamage + _processedDamage) min ([[0.99, 1] select KH_var_allowHitPointMaximumDamage, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
								};
							}
						] call KH_fnc_addEventHandler;

						[
							[_unit],
							{
								params ["_unit"];
								
								_unit addEventHandler [
									"HandleHeal", 
									{
										params ["_injured", "_healer", "_isMedic", "_atVehicle"];
										
										if (!(_injured getVariable ["KH_var_beingTreated", false]) && !(_healer getVariable ["KH_var_isTreating", false])) then {
											private _isSelfHeal = _injured isEqualTo _healer;
											_injured setVariable ["KH_var_beingTreated", true, true];
											_healer setVariable ["KH_var_isTreating", true, true];

											[
												[_injured, _healer, _isMedic, _atVehicle, _isSelfHeal, damage _injured],
												{
													params ["_injured", "_healer", "_isMedic", "_atVehicle", "_isSelfHeal", "_oldDamage"];

													if ((damage _injured) < _oldDamage) then {
														isNil {
															private _damageOffset = if !(isNull _atVehicle) then {
																1;
															}
															else {
																if KH_var_batchHeal then {
																	private _healItemCount = ([items _healer, "FirstAidKit"] call KH_fnc_countArrayElements) + 1;
																	private _currentDamageOffset = 0;

																	for "_i" from 1 to _healItemCount do {
																		_currentDamageOffset = _currentDamageOffset + ([
																			[
																				[KH_var_firstAidKitHeal, KH_var_firstAidKitHealWithstanding] select (_injured getVariable ["KH_var_withstanding", false]), 
																				[KH_var_firstAidKitSelfHeal, KH_var_firstAidKitSelfHealWithstanding] select (_healer getVariable ["KH_var_withstanding", false])
																			] select _isSelfHeal, 
																			[
																				[KH_var_firstAidKitHealMedic, KH_var_firstAidKitHealWithstandingMedic] select (_injured getVariable ["KH_var_withstanding", false]), 
																				[KH_var_firstAidKitSelfHealMedic, KH_var_firstAidKitSelfHealWithstandingMedic] select (_healer getVariable ["KH_var_withstanding", false])
																			] select _isSelfHeal
																		] select _isMedic);

																		if (_i isNotEqualTo 1) then {
																			_healer removeItem "FirstAidKit";
																		};

																		if (_currentDamageOffset >= 1) then {
																			break;
																		};
																	};

																	_currentDamageOffset min 1;
																}
																else {
																	([[KH_var_firstAidKitHeal, KH_var_firstAidKitSelfHeal] select _isSelfHeal, [KH_var_firstAidKitHealMedic, KH_var_firstAidKitSelfHealMedic] select _isSelfHeal] select _isMedic) min 1;
																};
															};

															_injured setDamage (([_oldDamage - _damageOffset, 0] select ((_oldDamage - _damageOffset) <= 0.25)) max 0);
															private _damages = getAllHitPointsDamage _injured;

															if ((damage _injured) isEqualTo 0) then {
																_damageOffset = 1;
															};

															{
																_injured setHitPointDamage [_x, (((_damages select 2) select _forEachIndex) - _damageOffset) max 0];
															} forEach (_damages select 0);
														};
													};

													_injured setVariable ["KH_var_beingTreated", false, true];
													_healer setVariable ["KH_var_isTreating", false, true];
												},
												true,
												{
													private _injured = param [0];
													private _oldDamage = param [5];
													((damage _injured) isNotEqualTo _oldDamage);
												},
												false
											] call KH_fnc_execute;

											nil;
										}
										else {
											true;
										};
									}
								];
							},
							"GLOBAL",
							true,
							false
						] call KH_fnc_execute;
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