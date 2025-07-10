private "_return";

isNil {
	_return = (
		[_x] apply {
			if (((((count _this) > 2) && !(isNil {param [2, nil]})) || (((count _this) < 3))) && !(isNil {param [1, nil]}) && !(isNil {param [0, nil]})) then {
				params ["_arguments", "_function", ["_target", "GLOBAL"], ["_environment", "THIS_FRAME"]];

				if (!(_function isEqualType {}) && !(_function isEqualType "")) exitWith {
					false;
				};

				private _subfunction = {
					params ["_arguments", "_function", "_target", "_override"];
					_function = [_function] call KH_fnc_parseFunction;
					
					switch (typeName _target) do {
						case "OBJECT": {
							if !(isNull _target) then {
								if (local _target) then {
									[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
								}
								else {
									["KH_eve_executionGlobal", [_arguments, _function, clientOwner], _target] call CBA_fnc_targetEvent;
								};

								true;
							}
							else {
								false;
							};
						};

						case "TEAM_MEMBER": {
							if !(isNull _target) then {
								if (local (agent _target)) then {
									[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
								}
								else {
									["KH_eve_executionGlobal", [_arguments, _function, clientOwner], agent _target] call CBA_fnc_targetEvent;
								};

								true;
							}
							else {
								false;
							};
						};

						case "GROUP": {
							if !(isNull _target) then {
								if (local _target) then {
									[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
								}
								else {
									[
										"KH_eve_executionServer", 
										[
											[_arguments, _function, _target, clientOwner],
											[
												{
													params ["_arguments", "_function", "_target", "_clientOwner"];
													["KH_eve_executionGlobal", [_arguments, _function, _clientOwner], groupOwner _target] call CBA_fnc_ownerEvent;
												} 
											] call KH_fnc_parseFunction,
											clientOwner
										]
									] call CBA_fnc_serverEvent;
								};

								true;
							}
							else {
								false;
							};
						};
						
						case "SCALAR": {
							if (_target == clientOwner) then {
								[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
								true;
							}
							else {
								if !(_target < 0) then {
									if (_target in KH_var_allMachines) then {
										["KH_eve_executionGlobal", [_arguments, _function, clientOwner], _target] call CBA_fnc_ownerEvent;
										true;
									}
									else {
										false;
									};
								}
								else {
									private _invertedTarget = abs _target;

									if (_invertedTarget == clientOwner) then {
										["KH_eve_executionGlobal", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
										true;
									}
									else {
										if (_invertedTarget in KH_var_allMachines) then {
											{
												["KH_eve_executionGlobal", [_arguments, _function, clientOwner], _x] call CBA_fnc_ownerEvent;
											} forEach (KH_var_allMachines - [_invertedTarget]);

											true;
										}
										else {
											false;
										};
									};
								};
							};
						};

						case "SIDE": {
							["KH_eve_executionPlayer", [_arguments, _function, clientOwner], [[_target], true] call KH_fnc_getClients] call CBA_fnc_targetEvent;
							true;
						};

						case "STRING": {
							switch _target do {
								case "SERVER": {
									if isServer then {
										[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
									}
									else {
										["KH_eve_executionServer", [_arguments, _function, clientOwner]] call CBA_fnc_serverEvent;
									};

									true;
								};

								case "GLOBAL": {
									["KH_eve_executionGlobal", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
									true;
								};

								case "PLAYERS": {
									["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
									true;
								};

								case "PLAYER_UNITS": {
									{
										["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case "HEADLESS": {
									["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
									true;
								};

								case "CURATORS": {
									{
										private _curatorUnit = getAssignedCuratorUnit _x;

										if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
											["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _curatorUnit] call CBA_fnc_targetEvent;
										};
									} forEach allCurators;

									true;
								};

								case "ADMIN": {
									if (KH_var_currentAdmin != 2) then {
										if (KH_var_currentAdmin == clientOwner) then {
											[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
										}
										else {
											["KH_eve_executionPlayer", [_arguments, _function, clientOwner], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
										};

										true;
									}
									else {
										false;
									};
								};

								case "LOCAL": {
									[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
								};

								default {
									if (((parseNumber (_target select [0, 1])) != 0) && ([_target] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})) then {
										if !(":" in _target) then {
											private _player = KH_var_allPlayerUidMachines get _target;

											if !(isNil "_player") then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _player] call CBA_fnc_ownerEvent;
												true;
											}
											else {
												_player = KH_var_allPlayerIdMachines get _target;

												if !(isNil "_player") then {
													["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _player] call CBA_fnc_ownerEvent;
													true;
												}
												else {
													private _client = KH_var_allHeadlessIdMachines get _target;

													if !(isNil "_client") then {
														["KH_eve_executionHeadless", [_arguments, _function, clientOwner], _client] call CBA_fnc_ownerEvent;
														true;
													}
													else {
														false;
													};
												};
											};
										}
										else {
											if !(isNull (objectFromNetId _target)) then {
												if (local (objectFromNetId _target)) then {
													[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
												}
												else {
													["KH_eve_executionGlobal", [_arguments, _function, clientOwner], objectFromNetId _target] call CBA_fnc_targetEvent;
												};

												true;
											}
											else {
												if !(isNull (groupFromNetId _target)) then {
													if (local (groupFromNetId _target)) then {
														[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
													}
													else {
														[
															"KH_eve_executionServer", 
															[
																[_arguments, _function, _target, clientOwner],
																[
																	{
																		params ["_arguments", "_function", "_target", "_clientOwner"];
																		["KH_eve_executionGlobal", [_arguments, _function, _clientOwner], groupOwner (groupFromNetId _target)] call CBA_fnc_ownerEvent;
																	} 
																] call KH_fnc_parseFunction,
																clientOwner
															]
														] call CBA_fnc_serverEvent;
													};

													true;
												}
												else {
													false;
												};											
											};
										};
									}
									else {
										private _unitPresent = false;

										{
											if ((name _x) == _target) then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
												_unitPresent = true;
												break;
											};
										} forEach KH_var_allPlayerUnits;
										
										_unitPresent;
									};
								};
							};
						};
						
						case "ARRAY": {
							private _type = _target select 0;

							switch _type do {
								case "GROUP_PLAYERS": {
									private _group = _target select 1;
									private _invert = _target param [2, false];

									if !(isNull _group) then {
										{
											if !_invert then {
												if ((group _x) == _group) then {
													["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
												};
											}
											else {
												if ((group _x) != _group) then {
													["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
												};
											};
										} forEach KH_var_allPlayerUnits;

										true;
									}
									else {
										false;
									};
								};

								case "PLAYER_ROLE": {
									private _role = _target select 1;
									private _invert = _target param [2, false];
									
									{
										if !_invert then {
											if ((roleDescription _x) == _role) then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
											};
										}
										else {
											if ((roleDescription _x) != _role) then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
											};
										};
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case "AREA_PLAYERS": {
									private _area = _target select 1;
									private _invert = _target param [2, false];
									
									{
										if _invert then {
											if !(_x inArea _area) then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
											};
										}
										else {
											if (_x inArea _area) then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
											};
										};
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case "CONDITION": {
									private _conditionArguments = _target select 1;
									private _conditionFunction = _target select 2;
									private _exclusiveType = _target param [3, "GLOBAL"];
									private _eventType = "";

									switch _exclusiveType do {
										case "GLOBAL": {
											_eventType = "KH_eve_executionGlobal";
										};

										case "SERVER": {
											_eventType = "KH_eve_executionServer";
										};

										case "PLAYERS": {
											_eventType = "KH_eve_executionPlayer";
										};

										case "HEADLESS": {
											_eventType = "KH_eve_executionHeadless";
										};
									};

									[
										_eventType, 
										[
											[_arguments, _function, _conditionArguments, _conditionFunction, clientOwner],
											[
												{
													params ["_arguments", "_function", "_conditionArguments", "_conditionFunction", "_clientOwner"];
													_conditionFunction = [_conditionFunction] call KH_fnc_parseFunction;
													
													if ([_conditionArguments, _conditionFunction, clientOwner] call KH_fnc_callParsedFunction) then {
														[_arguments, _function, _clientOwner] call KH_fnc_callParsedFunction;
													};
												}
										 	] call KH_fnc_parseFunction,
											clientOwner
										]
									] call CBA_fnc_globalEvent;

									true;
								};

								case "PERFORMANT_MACHINE": {
									private _playersOnly = _target param [1, false];
									private _invert = _target param [2, false];
									private _framerates = [missionNamespace, "KH_var_machineFramerates", createHashMap, false] call KH_fnc_atomicVariable;
									private _id = [] call KH_fnc_generateUid;
									private _machines = [];

									private _callbackHandler = [
										"PUBLIC_VARIABLE",
										_id,
										[_framerates],
										{
											_args params ["_framerates"];
											private _framerateList = missionNamespace getVariable [_framerates, createHashMap];
											_framerateList insert [_this select 1];
											missionNamespace setVariable [_framerates, _framerateList];
										}
									] call KH_fnc_addEventHandler;

									if _playersOnly then {
										_machines = KH_var_allPlayerMachines;
									}
									else {
										_machines = KH_var_allMachines;
									};

									{
										[
											"KH_eve_executionGlobal", 
											[
												[_id, clientOwner],
												[
													{
														params ["_id", "_clientOwner"];
														private _framerate = 0;

														if !hasInterface then {
															if (diag_fpsMin > 45) then {
																_framerate = 999999;
															};
														}
														else {
															_framerate = diag_fpsMin;
														};

														missionNamespace setVariable [_id, [_framerate, clientOwner], _clientOwner];
													}
											 	] call KH_fnc_parseFunction,
												clientOwner
											], 
											_x
										] call CBA_fnc_ownerEvent;
									} forEach _machines;

									private _chooserFunction = {
										params ["_arguments", "_function", "_invert", "_framerates", "_callbackHandler"];
										private _framerateList = missionNamespace getVariable [_framerates, createHashMap];
										private _parsedFramerates = [];

										{
											_parsedFramerates pushBack _x;
										} forEach _framerateList;
										
										private _chosenMachine = 2;

										if _invert then {
											_chosenMachine = _framerateList get (selectMin _parsedFramerates);
										}
										else {
											_chosenMachine = _framerateList get (selectMax _parsedFramerates);
										};

										["KH_eve_executionGlobal", [_arguments, _function, clientOwner], _chosenMachine] call CBA_fnc_ownerEvent;
										[_callbackHandler] call KH_fnc_removeEventHandler;
									};

									[
										{
											private _framerates = _this select 3;
											private _machineCount = _this select 5;
											((count ((toArray (missionNamespace getVariable [_framerates, createHashMap])) select 0)) == _machineCount);
										}, 
										_chooserFunction,
										[_arguments, _function, _invert, _framerates, _callbackHandler, count _machines],
										1,
										_chooserFunction
									] call CBA_fnc_waitUntilAndExecute;

									true;
								};

								case "MATCHING_MACHINES": {
									private _invert = _target param [1, false];

									if !_invert then {
										if hasInterface then {
											["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
										}
										else {
											if !isServer then {
												["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
											}
											else {
												[_arguments, _function, clientOwner] call KH_fnc_callParsedFunction;
											};
										};
									}
									else {
										if !hasInterface then {
											if !isServer then {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
												["KH_eve_executionServer", [_arguments, _function, clientOwner]] call CBA_fnc_serverEvent;
											}
											else {
												["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
												["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
											};
										}
										else {
											["KH_eve_executionServer", [_arguments, _function, clientOwner]] call CBA_fnc_serverEvent;
											["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
										};
									};
									
									true;
								};

								case "REMOTE": {
									private _exclusiveType = _target param [1, "GLOBAL"];
									
									switch (typeName _exclusiveType) do {
										case "STRING": {
											switch _exclusiveType do {
												case "GLOBAL": {
													["KH_eve_executionGlobal", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
												};

												case "PLAYERS": {
													["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
												};

												case "PLAYER_UNITS": {
													{
														["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
													} forEach (KH_var_allPlayerUnits - [player]);
												};

												case "CURATORS": {
													{
														private _curatorUnit = getAssignedCuratorUnit _x;

														if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
															["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _curatorUnit] call CBA_fnc_targetEvent;
														};
													} forEach (allCurators - [getAssignedCuratorLogic player]);
												};

												case "HEADLESS": {
													["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
												};
											};

											true;
										};

										case "ARRAY": {
											private _targetType = _exclusiveType select 0;

											switch _targetType do {
												case "GROUP_PLAYERS": {
													private _group = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];

													if !(isNull _group) then {
														{
															if !_invert then {
																if ((group _x) == _group) then {
																	["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
																};
															}
															else {
																if ((group _x) != _group) then {
																	["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
																};
															};
														} forEach (KH_var_allPlayerUnits - [player]);

														true;
													}
													else {
														false;
													};
												};

												case "PLAYER_ROLE": {
													private _role = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];
													
													{
														if !_invert then {
															if ((roleDescription _x) == _role) then {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
															};
														}
														else {
															if ((roleDescription _x) != _role) then {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
															};
														};
													} forEach (KH_var_allPlayerUnits - [player]);

													true;
												};

												case "AREA_PLAYERS": {
													private _area = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];
													
													{
														if _invert then {
															if !(_x inArea _area) then {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
															};
														}
														else {
															if (_x inArea _area) then {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner], _x] call CBA_fnc_targetEvent;
															};
														};
													} forEach (KH_var_allPlayerUnits - [player]);

													true;
												};

												case "CONDITION": {
													private _conditionArguments = _exclusiveType select 1;
													private _conditionFunction = _exclusiveType select 2;
													private _exclusiveType = _exclusiveType param [3, "GLOBAL"];
													private _eventType = "";

													switch _exclusiveType do {
														case "GLOBAL": {
															_eventType = "KH_eve_executionGlobal";
														};

														case "SERVER": {
															_eventType = "KH_eve_executionServer";
														};

														case "PLAYERS": {
															_eventType = "KH_eve_executionPlayer";
														};

														case "HEADLESS": {
															_eventType = "KH_eve_executionHeadless";
														};
													};

													[
														_eventType, 
														[
															[_arguments, _function, _conditionArguments, _conditionFunction, clientOwner], 
															[
																{
																	params ["_arguments", "_function", "_conditionArguments", "_conditionFunction", "_clientOwner"];
																	_conditionFunction = [_conditionFunction] call KH_fnc_parseFunction;
																	
																	if ([_conditionArguments, _conditionFunction, clientOwner] call KH_fnc_callParsedFunction) then {
																		[_arguments, _function, _clientOwner] call KH_fnc_callParsedFunction;
																	};
																}
															] call KH_fnc_parseFunction,
															clientOwner
														]
													] call CBA_fnc_remoteEvent;

													true;
												};

												case "MATCHING_MACHINES": {
													private _invert = _exclusiveType param [1, false];

													if !_invert then {
														if hasInterface then {
															["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
														}
														else {
															if !isServer then {
																["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_remoteEvent;
															};
														};
													}
													else {
														if !hasInterface then {
															if !isServer then {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
																["KH_eve_executionServer", [_arguments, _function, clientOwner]] call CBA_fnc_serverEvent;
															}
															else {
																["KH_eve_executionPlayer", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
																["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
															};
														}
														else {
															["KH_eve_executionServer", [_arguments, _function, clientOwner]] call CBA_fnc_serverEvent;
															["KH_eve_executionHeadless", [_arguments, _function, clientOwner]] call CBA_fnc_globalEvent;
														};
													};

													true;
												};

												default {
													false;
												};
											};											
										};
									};
								};

								case "TARGETS": {
									private _targets = _target select 1;

									[
										"KH_eve_executionServer",
										[
											[_arguments, _function, _targets, clientOwner, isServer, hasInterface],
											[
												{
													params ["_arguments", "_function", "_targets", "_caller", "_isServer", "_hasInterface"];
													private _exclusiveList = [];

													{
														switch (typeName _x) do {
															case "OBJECT": {
																if !(isNull _x) then {
																	if !((owner _x) in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																		_exclusiveList pushBack (owner _x);
																	};
																};
															};

															case "TEAM_MEMBER": {
																if !(isNull _x) then {
																	private _agent = agent _x;

																	if !((owner _agent) in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function, _caller], _agent] call CBA_fnc_targetEvent;
																		_exclusiveList pushBack (owner _agent);
																	};
																};
															};

															case "GROUP": {															
																if !(isNull _x) then {
																	if !((groupOwner _x) in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function, _caller], groupOwner _x] call CBA_fnc_ownerEvent;
																		_exclusiveList pushBack (groupOwner _x);
																	};
																};
															};
															
															case "SCALAR": {
																if (_x in KH_var_allMachines) then {
																	if !(_x in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																		_exclusiveList pushBack _x;
																	};
																};
															};

															case "SIDE": {
																private _side = _x;

																{
																	if !((owner _x) in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																		_exclusiveList pushBack (owner _x);
																	};
																} forEach ([[_side], true] call KH_fnc_getClients);
															};
															
															case "STRING": {
																switch _x do {
																	case "SERVER": {
																		if !(2 in _exclusiveList) then {
																			[_arguments, _function, _caller] call KH_fnc_callParsedFunction;
																			_exclusiveList pushBack 2;
																		};
																	};

																	case "PLAYERS": {
																		{
																			if !(_x in _exclusiveList) then {
																				["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																				_exclusiveList pushBack _x;
																			};
																		} forEach KH_var_allPlayerMachines;
																	};

																	case "PLAYER_UNITS": {
																		{
																			if !((owner _x) in _exclusiveList) then {
																				["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																				_exclusiveList pushBack (owner _x);
																			};
																		} forEach KH_var_allPlayerUnits;
																	};

																	case "HEADLESS": {
																		{
																			if !(_x in _exclusiveList) then {
																				["KH_eve_executionHeadless", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																				_exclusiveList pushBack _x;
																			};
																		} forEach KH_var_allHeadlessMachines;
																	};

																	case "CURATORS": {
																		{
																			private _curatorUnit = getAssignedCuratorUnit _x;

																			if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
																				if !((owner _curatorUnit) in _exclusiveList) then {
																					["KH_eve_executionPlayer", [_arguments, _function, _caller], _curatorUnit] call CBA_fnc_targetEvent;
																					_exclusiveList pushBack (owner _curatorUnit);
																				};
																			};
																		} forEach allCurators;
																	};

																	case "ADMIN": {
																		if (KH_var_currentAdmin != 2) then {
																			if !(KH_var_currentAdmin in _exclusiveList) then {
																				["KH_eve_executionPlayer", [_arguments, _function, _caller], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
																				_exclusiveList pushBack KH_var_currentAdmin;
																			};
																		};
																	};

																	case "LOCAL": {
																		if !(_caller in _exclusiveList) then {
																			["KH_eve_executionGlobal", [_arguments, _function, _caller], _caller] call CBA_fnc_ownerEvent;
																			_exclusiveList pushBack _caller;
																		};
																	};

																	default {
																		if (((parseNumber (_x select [0, 1])) != 0) && ([_x] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})) then {
																			if !(":" in _x) then {
																				private _player = KH_var_allPlayerUidMachines get _x;

																				if !(isNil "_player") then {
																					if !(_player in _exclusiveList) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _player] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack _player;
																					};
																				}
																				else {
																					_player = KH_var_allPlayerIdMachines get _x;

																					if !(isNil "_player") then {
																						if !(_player in _exclusiveList) then {
																							["KH_eve_executionPlayer", [_arguments, _function, _caller], _player] call CBA_fnc_ownerEvent;
																							_exclusiveList pushBack _player;
																						};
																					}
																					else {
																						private _client = KH_var_allHeadlessIdMachines get _x;

																						if !(isNil "_client") then {
																							if !(_client in _exclusiveList) then {
																								["KH_eve_executionHeadless", [_arguments, _function, _caller], _client] call CBA_fnc_ownerEvent;
																								_exclusiveList pushBack _client;
																							};
																						};
																					};
																				};
																			}
																			else {
																				if !(isNull (objectFromNetId _x)) then {
																					["KH_eve_executionGlobal", [_arguments, _function, _caller], objectFromNetId _x] call CBA_fnc_targetEvent;
																					_exclusiveList pushBack (owner (objectFromNetId _x));
																				}
																				else {
																					if !(isNull (groupFromNetId _x)) then {
																						["KH_eve_executionGlobal", [_arguments, _function, _caller], groupOwner (groupFromNetId _target)] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack (groupOwner (groupFromNetId _x));
																					};											
																				};
																			};
																		}
																		else {
																			private _currentTarget = _x;

																			{
																				if ((name _x) == _currentTarget) then {
																					if !((owner _x) in _exclusiveList) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																						_exclusiveList pushBack (owner _x);
																					};

																					break;
																				};
																			} forEach KH_var_allPlayerUnits;
																		};
																	};
																};
															};

															case "ARRAY": {
																private _currentTarget = _x;
																private _targetType = _currentTarget select 0;

																switch _targetType do {
																	case "GROUP_PLAYERS": {
																		private _group = _currentTarget select 1;
																		private _invert = _currentTarget param [2, false];
																	
																		if !(isNull _group) then {
																			{
																				if !((owner _x) in _exclusiveList) then {
																					if !_invert then {
																						if ((group _x) == _group) then {
																							["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																						};
																					}
																					else {
																						if ((group _x) != _group) then {
																							["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																						};
																					};

																					_exclusiveList pushBack (owner _x);
																				};
																			} forEach KH_var_allPlayerUnits;
																		};
																	};

																	case "PLAYER_ROLE": {
																		private _role = _currentTarget select 1;
																		private _invert = _currentTarget param [2, false];
																		
																		{
																			if !((owner _x) in _exclusiveList) then {
																				if !_invert then {
																					if ((roleDescription _x) == _role) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																					};
																				}
																				else {
																					if ((roleDescription _x) != _role) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																					};
																				};

																				_exclusiveList pushBack (owner _x);
																			};
																		} forEach KH_var_allPlayerUnits;
																	};

																	case "AREA_PLAYERS": {
																		private _area = _currentTarget select 1;
																		private _invert = _currentTarget param [2, false];
																		
																		{
																			if !((owner _x) in _exclusiveList) then {
																				if _invert then {
																					if !(_x inArea _area) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																						_exclusiveList pushBack (owner _x);
																					};
																				}
																				else {
																					if (_x inArea _area) then {
																						["KH_eve_executionPlayer", [_arguments, _function, _caller], _x] call CBA_fnc_targetEvent;
																						_exclusiveList pushBack (owner _x);
																					};
																				};
																			};
																		} forEach KH_var_allPlayerUnits;
																	};

																	case "MATCHING_MACHINES": {
																		private _invert = _currentTarget param [1, false];

																		if !_invert then {
																			if _hasInterface then {
																				{
																					["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																					_exclusiveList pushBack _x;
																				} forEach KH_var_allPlayerMachines;
																			}
																			else {
																				if !_isServer then {
																					{
																						["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack _x;
																					} forEach KH_var_allHeadlessMachines;
																				}
																				else {
																					[_arguments, _function, _caller] call KH_fnc_callParsedFunction;
																					_exclusiveList pushBack 2;
																				};
																			};
																		}
																		else {
																			if !_hasInterface then {
																				if !_isServer then {
																					{
																						["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack _x;
																					} forEach (KH_var_allPlayerMachines + [2]);
																				}
																				else {
																					{
																						["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack _x;
																					} forEach (KH_var_allPlayerMachines + KH_var_allHeadlessMachines);
																				};
																			}
																			else {
																				{
																					["KH_eve_executionGlobal", [_arguments, _function, _caller], _x] call CBA_fnc_ownerEvent;
																					_exclusiveList pushBack _x;
																				} forEach (KH_var_allHeadlessMachines + [2]);
																			};
																		};
																	};
																};
															};
														};
													} forEach _targets;
												}
										 	] call KH_fnc_parseFunction,
											clientOwner
										]
									] call CBA_fnc_serverEvent;

									true;
								};

								case "JIP": {
									private _exclusiveType = _target param [1, "GLOBAL"];
									private _dependency = _target param [2, true];
									private _unitRequired = _target param [3, true];
									private _remote = _target param [4, false];
									private _overrideId = _target param [5, ""];
									private _id = "";

									if (_overrideId != "") then {
										_id = _overrideId;
									}
									else {
										if (_override != "") then {
											_id = _override;
										}
										else {
											_id = [] call KH_fnc_generateUid;
										};
									};

									private _jipOverrideParams = format ["KH_var_jipOverrideParams%1", _id];
									private _jipOverrideId = format ["KH_var_jipOverrideId%1", _id];
									private _jipOverrideType = format ["KH_var_jipOverrideType%1", _id];

									[
										"KH_eve_executionServer",
										[
											[_arguments, _function, _exclusiveType, _dependency, _unitRequired, _remote, _id, _jipOverrideParams, _jipOverrideId, _jipOverrideType, clientOwner],
											[
												{
													params ["_arguments", "_function", "_exclusiveType", "_dependency", "_unitRequired", "_remote", "_id", "_jipOverrideParams", "_jipOverrideId", "_jipOverrideType", "_originalOwner"];

													switch _exclusiveType do {
														case "GLOBAL": {
															if !_remote then {
																if !_unitRequired then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																	} forEach KH_var_allMachines;
																}
																else {
																	{
																		["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_targetEvent;
																	} forEach KH_var_allPlayerUnits;
																	
																	if isDedicated then {
																		{
																			["KH_eve_executionGlobal", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																		} forEach ([2] + KH_var_allHeadlessMachines);
																	}
																	else {
																		{
																			["KH_eve_executionHeadless", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																		} forEach KH_var_allHeadlessMachines;
																	};
																};
															}
															else {
																if !_unitRequired then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																	} forEach (KH_var_allMachines - [_originalOwner]);
																}
																else {
																	{
																		if ((owner _x) != _originalOwner) then {
																			["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_targetEvent;
																		};
																	} forEach KH_var_allPlayerUnits;

																	if isDedicated then {
																		{
																			["KH_eve_executionGlobal", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																		} forEach (([2] + KH_var_allHeadlessMachines) - [_originalOwner]);
																	}
																	else {
																		{
																			["KH_eve_executionHeadless", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																		} forEach (KH_var_allHeadlessMachines - [_originalOwner]);
																	};
																};
															};
														};

														case "PLAYERS": {
															if !_remote then {
																if !_unitRequired then {
																	{
																		["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																	} forEach KH_var_allPlayerMachines;
																}
																else {
																	{
																		["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_targetEvent;
																	} forEach KH_var_allPlayerUnits;
																};
															}
															else {
																if !_unitRequired then {
																	{
																		["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																	} forEach (KH_var_allPlayerMachines - [_originalOwner]);
																}
																else {
																	{
																		if ((owner _x) != _originalOwner) then {
																			["KH_eve_executionPlayer", [_arguments, _function, _originalOwner], _x] call CBA_fnc_targetEvent;
																		};
																	} forEach KH_var_allPlayerUnits;
																};
															};
														};

														case "HEADLESS": {
															if !_remote then {
																{
																	["KH_eve_executionHeadless", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																} forEach KH_var_allHeadlessMachines;
															}
															else {
																{
																	["KH_eve_executionHeadless", [_arguments, _function, _originalOwner], _x] call CBA_fnc_ownerEvent;
																} forEach (KH_var_allHeadlessMachines - [_originalOwner]);
															};
														};
													};

													private _joinType = [];
													
													if _unitRequired then {
														switch _exclusiveType do {
															case "GLOBAL": {
																_joinType = ["KH_eve_playerLoaded", "KH_eve_headlessPreloaded"];
															};

															case "PLAYERS": {
																_joinType = ["KH_eve_playerLoaded"];
															};

															case "HEADLESS": {
																_joinType = ["KH_eve_headlessPreloaded"];
															};

															case "JIP_GLOBAL": {
																_joinType = ["KH_eve_playerLoaded", "KH_eve_headlessPreloaded"];
															};

															case "JIP_PLAYERS": {
																_joinType = ["KH_eve_playerLoaded"];
															};

															case "JIP_HEADLESS": {
																_joinType = ["KH_eve_headlessPreloaded"];
															};
														};
													}
													else {
														switch _exclusiveType do {
															case "GLOBAL": {
																_joinType = ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
															};

															case "PLAYERS": {
																_joinType = ["KH_eve_playerPreloadedInitial"];
															};

															case "HEADLESS": {
																_joinType = ["KH_eve_headlessPreloaded"];
															};

															case "JIP_GLOBAL": {
																_joinType = ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
															};

															case "JIP_PLAYERS": {
																_joinType = ["KH_eve_playerPreloadedInitial"];
															};

															case "JIP_HEADLESS": {
																_joinType = ["KH_eve_headlessPreloaded"];
															};
														};
													};

													missionNamespace setVariable [_jipOverrideParams, [_arguments, _function, _dependency]];

													if ((missionNamespace getVariable [_jipOverrideType, []]) isNotEqualTo [_exclusiveType, _unitRequired]) then {
														missionNamespace setVariable [_jipOverrideId, false];
														private _oldHandlers = KH_var_jipEventHandlers get _id;

														if !(isNil "_oldHandlers") then {
															{
																[_x] call KH_fnc_removeEventHandler;
															} forEach _oldHandlers;

															KH_var_jipEventHandlers deleteAt _id;
														};
													};
													
													if !(missionNamespace getVariable [_jipOverrideId, false]) then {
														missionNamespace setVariable [_jipOverrideType, [_exclusiveType, _unitRequired]];
														private _currentHandlers = [];

														{
															private _currentHandler = [
																"CBA",
																_x,
																[_id, _jipOverrideParams, _x],
																{
																	_args params ["_id", "_jipOverrideParams", "_joinType"];
																	private _idState = missionNamespace getVariable [_id, "ACTIVE"];
																	private _joiningMachine = 999999;

																	if (_joinType == "KH_eve_playerLoaded") then {
																		_joiningMachine = _this select 1;
																	}
																	else {
																		_joiningMachine = _this select 0;
																	};

																	(missionNamespace getVariable [_jipOverrideParams, []]) params ["_arguments", "_function", "_dependency"];
																	
																	if (_idState isNotEqualTo "INACTIVE") then {
																		switch _idState do {
																			case "ACTIVE": {
																				private _condition = true;

																				switch (typeName _dependency) do {
																					case "STRING": {
																						if ((parseNumber (_dependency select [0, 1])) != 0) then {
																							private _uid = "";

																							if (_joinType == "KH_eve_playerLoaded") then {
																								_uid = getPlayerUID (_this select 0); 
																							}
																							else {
																								_uid = _this select 1;
																							};

																							if (_dependency != _uid) then {
																								if (":" in _dependency) then {
																									if ((isNull (objectFromNetId _dependency)) && (isNull (groupFromNetId _dependency))) then {
																										_condition = false;
																									};
																								}
																								else {
																									private _player = KH_var_allPlayerIdMachines get _x;
																									private _client = KH_var_allHeadlessIdMachines get _x;

																									if ((isNil "_player") && (isNil "_client")) then {
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

																					case "SCALAR": {
																						if !(_dependency in KH_var_allMachines) then {
																							_condition = false;
																						};
																					};

																					case "CODE": {
																						if !([] call _dependency) then {
																							_condition = false;
																						};
																					};

																					case "ARRAY": {																					
																						{
																							switch (typeName _x) do {
																								case "STRING": {
																									if ((parseNumber (_x select [0, 1])) != 0) then {
																										private _uid = "";

																										if (_joinType == "KH_eve_playerLoaded") then {
																											_uid = getPlayerUID (_this select 0); 
																										}
																										else {
																											_uid = _this select 1;
																										};

																										if (_x != _uid) then {
																											if (":" in _x) then {
																												if ((isNull (objectFromNetId _x)) && (isNull (groupFromNetId _x))) then {
																													_condition = false;
																													break;
																												};
																											}
																											else {
																												private _player = KH_var_allPlayerIdMachines get _x;
																												private _client = KH_var_allHeadlessIdMachines get _x;

																												if ((isNil "_player") && (isNil "_client")) then {
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

																								case "SCALAR": {
																									if !(_x in KH_var_allMachines) then {
																										_condition = false;
																										break;
																									};
																								};

																								case "CODE": {
																									if !([] call _x) then {
																										_condition = false;
																										break;
																									};
																								};
																							};
																						} forEach _dependency;
																					};

																					case "BOOL": {
																						if !_dependency then {
																							_condition = false;
																						};
																					};
																				};

																				if _condition then {
																					["KH_eve_executionGlobal", [_arguments, _function, _originalOwner], _joiningMachine] call CBA_fnc_ownerEvent;
																				};
																			};

																			case "TERMINATE": {
																				[_localId] call KH_fnc_removeEventHandler;
																			};		
																		};
																	};
																}
															] call KH_fnc_addEventHandler;

															_currentHandlers pushBack _currentHandler;
														} forEach _joinType;

														missionNamespace setVariable [_jipOverrideId, true];
														KH_var_jipEventHandlers set [_id, _currentHandlers, false];
													};
												} 
											] call KH_fnc_parseFunction,
											clientOwner
										]
									] call CBA_fnc_serverEvent;

									["JIP_HANDLER", _id];
								};

								case "CALLBACK": {
									private _callbackTarget = _target select 1;
									private _callbackArguments = _target select 2;
									private _callbackFunction = _target select 3;
									private _id = [] call KH_fnc_generateUid;

									[
										"PUBLIC_VARIABLE",
										_id,
										[_arguments, _function],
										{
											_args params ["_arguments", "_function"];
											private _argsCallback = [] call KH_fnc_generateUid;
											missionNamespace setVariable [_argsCallback, _this select 1];

											private _expression = [
												"private _argsCallback = (missionNamespace getVariable ['", _argsCallback, "', []]);
												call ", [_arguments, _function] call KH_fnc_getParsedFunction, ";"
											] joinString "";

											_arguments call (compile _expression);
											[_localId] call KH_fnc_removeEventHandler;
										}
									] call KH_fnc_addEventHandler;

									switch (typeName _callbackTarget) do {
										case "STRING": {
											switch _callbackTarget do {
												case "SERVER": {
													["KH_eve_executionServer", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner]] call CBA_fnc_serverEvent;
													true;
												};

												case "ADMIN": {
													if (KH_var_currentAdmin != 2) then {
														["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
														true;
													}
													else {
														false;
													};
												};

												default {
													if (((parseNumber (_callbackTarget select [0, 1])) != 0) && ([_callbackTarget] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})) then {
														if !(":" in _callbackTarget) then {
															private _player = KH_var_allPlayerUidMachines get _callbackTarget;

															if !(isNil "_player") then {
																["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _player] call CBA_fnc_ownerEvent;
																true;
															}
															else {
																_player = KH_var_allPlayerIdMachines get _callbackTarget;

																if !(isNil "_player") then {
																	["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _player] call CBA_fnc_ownerEvent;
																	true;
																}
																else {
																	private _client = KH_var_allHeadlessIdMachines get _callbackTarget;

																	if !(isNil "_client") then {
																		["KH_eve_executionHeadless", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _client] call CBA_fnc_ownerEvent;
																		true;
																	}
																	else {
																		false;
																	};
																};
															};
														}
														else {
															if !(isNull (objectFromNetId _callbackTarget)) then {
																["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], objectFromNetId _callbackTarget] call CBA_fnc_targetEvent;
																true;
															}
															else {
																if !(isNull (groupFromNetId _callbackTarget)) then {
																	[
																		"KH_eve_executionServer", 
																		[
																			[_callbackArguments, _callbackFunction, _callbackTarget, "KH_fnc_callback", _id, clientOwner],
																			[
																				{
																					params ["_callbackArguments", "_callbackFunction", "_callbackTarget", "_id", "_clientOwner"];
																					["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, _clientOwner], "KH_fnc_callback", _clientOwner], groupOwner (groupFromNetId _callbackTarget)] call CBA_fnc_ownerEvent;
																				}
																			] call KH_fnc_parseFunction,
																			clientOwner
																		]
																	] call CBA_fnc_serverEvent;

																	true;
																}
																else {
																	false;
																};											
															};
														};
													}
													else {
														private _unitPresent = false;

														{
															if ((name _x) == _callbackTarget) then {
																["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _x] call CBA_fnc_targetEvent;
																_unitPresent = true;
																break;
															};
														} forEach KH_var_allPlayerUnits;
														
														_unitPresent;
													};
												};
											};
										};

										case "OBJECT": {
											if !(isNull _callbackTarget) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _callbackTarget] call CBA_fnc_targetEvent;
												true;
											}
											else {
												false;
											};
										};

										case "GROUP": {
											if !(isNull _callbackTarget) then {
												[
													"KH_eve_executionServer", 
													[
														[_callbackArguments, _callbackFunction, _callbackTarget, "KH_fnc_callback", _id, clientOwner],
														[
															{
																params ["_callbackArguments", "_callbackFunction", "_callbackTarget", "_id", "_clientOwner"];
																["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, _clientOwner], "KH_fnc_callback", _clientOwner], groupOwner _callbackTarget] call CBA_fnc_ownerEvent;
															}
													 	] call KH_fnc_parseFunction,
														clientOwner
													]
												] call CBA_fnc_serverEvent;
												
												true;
											}
											else {
												false;
											};
										};

										case "TEAM_MEMBER": {
											if !(isNull _callbackTarget) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], agent _callbackTarget] call CBA_fnc_targetEvent;
												true;
											}
											else {
												false;
											};
										};

										case "SCALAR": {
											if (_callbackTarget in KH_var_allMachines) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _callbackTarget] call CBA_fnc_ownerEvent;
												true;
											}
											else {
												false;
											};
										};
									};
								};

								case "PERSISTENT": {
									private _targetObject = _target select 1;

									if !(isNull _targetObject) then {
										private _persistencyFunction = [
											{
												params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];

												[
													["STANDARD", _targetObject, false],
													"Local",
													[_arguments, _function, _sendoffArguments, _sendoffFunction, _originalOwner, _id],
													{
														params ["_targetObject", "_isLocal"];
														_args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];
														private _idState = _targetObject getVariable [_id, "ACTIVE"];

														if (_idState isNotEqualTo "INACTIVE") then {
															switch _idState do {
																case "ACTIVE": {
																	if _isLocal then {
																		[_arguments, _function, _originalOwner] call KH_fnc_callParsedFunction;
																	}
																	else {
																		[_sendoffArguments, _sendoffFunction, _originalOwner] call KH_fnc_callParsedFunction;
																	};
																};

																case "TERMINATE": {
																	[_localId] call KH_fnc_removeEventHandler;
																};
															};
														};
													}
												] call KH_fnc_addEventHandler;
											}
										] call KH_fnc_parseFunction;

										private _sendoffArguments = _target param [2, []];
										private _sendoffFunction = _target param [3, {}];
										private _jip = _target param [4, true];
										private _id = "";
										_sendoffFunction = [_sendoffFunction] call KH_fnc_parseFunction;

										if (_override != "") then {
											_id = _override;
										}
										else {
											_id = [] call KH_fnc_generateUid;
										};

										["KH_eve_executionGlobal", [_arguments, _function, clientOwner], _targetObject] call CBA_fnc_targetEvent;

										{
											[
												"KH_eve_executionGlobal",
												[
													[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, clientOwner, _id], 
													_persistencyFunction,
													clientOwner
												],
												_x
											] call CBA_fnc_ownerEvent;
										} forEach KH_var_allMachines;

										if _jip then {
											[
												"KH_eve_executionServer", 
												[
													[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, clientOwner, _id], 
													[
														{
															params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];

															{
																[
																	"CBA",
																	_x,
																	[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, _originalOwner, _id],
																	{
																		_args params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];
																		params ["_joiningMachine"];

																		[
																			"KH_eve_executionGlobal",
																			[
																				[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _originalOwner, _id], 
																				_persistencyFunction,
																				_originalOwner
																			],
																			_joiningMachine
																		] call CBA_fnc_ownerEvent;
																	}
																] call KH_fnc_addEventHandler;
															} forEach ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
														}
												 	] call KH_fnc_parseFunction,
													clientOwner
												]
											] call CBA_fnc_serverEvent;
										};

										["PERSISTENT_HANDLER", _id, _targetObject];
									}
									else {
										false;
									};
								};
								
								default {
									false;
								};
							};
						};

						default {
							false;
						};
					};
				};

				private _processOverride = {
					params ["_target", "_override", "_result"];
					private _type = _target select 0;

					switch _type do {
						case "JIP": {
							private _overrideId = _target param [5, ""];

							if (_overrideId != "") then {
								_override = _overrideId;
							}
							else {
								_override = [] call KH_fnc_generateUid;
							};

							_result = ["JIP_HANDLER", _override];
						};

						case "PERSISTENT": {
							_override = [] call KH_fnc_generateUid;
							_result = ["PERSISTENT_HANDLER", _override, _target select 1];
						};
					};

					[_override, _result];
				};

				switch (typeName _environment) do {
					case "STRING": {
						switch _environment do {
							case "THIS_FRAME": {
								private _override = "";

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _override] call _subfunction;
								}
								else {
									private _result = true;

									if (_target isEqualType []) then {
										private _overrideResult = [_target, _override, _result] call _processOverride;
										_override = _overrideResult select 0;
										_result = _overrideResult select 1;
									};

									KH_var_postInitExecutions pushBack [[_arguments, _function, _target, _override], _subfunction];
									_result;
								};
							};

							case "NEXT_FRAME": {
								private _result = true;
								private _override = "";
								
								if (_target isEqualType []) then {
									private _overrideResult = [_target, _override, _result] call _processOverride;
									_override = _overrideResult select 0;
									_result = _overrideResult select 1;
								};

								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_subfunction", "_override"];

									[
										{
											params ["_arguments", "_function", "_target", "_subfunction", "_override"];
											[_arguments, _function, _target, _override] call _subfunction;
										}, 
										[_arguments, _function, _target, _subfunction, _override]
									] call CBA_fnc_execNextFrame;
								};

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _subfunction, _override] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _subfunction, _override],
										_environmentCaller
									];
								};

								_result;
							};

							default {
								false;
							};
						};
					};

					case "SCALAR": {
						private _result = true;
						private _override = "";
						
						if (_target isEqualType []) then {
							private _overrideResult = [_target, _override, _result] call _processOverride;
							_override = _overrideResult select 0;
							_result = _overrideResult select 1;
						};

						private _environmentCaller = {
							params ["_arguments", "_function", "_target", "_override", "_subfunction", "_executionCount"];
							private _i = 1;
							private _results = [];

							for "_i" from 1 to _executionCount do {
								_results pushBack ([_arguments, _function, _target, _override] call _subfunction);
							};

							_results;
						};

						if KH_var_missionLoaded then {
							[[_arguments, _function, _target, _override, _subfunction] call _environmentCaller, _result];
						}
						else {
							KH_var_postInitExecutions pushBack [
								[_arguments, _function, _target, _override, _subfunction],
								_environmentCaller
							];

							[[], _result];
						};
					};

					case "ARRAY": {
						private _result = true;
						private _override = "";
						
						if (_target isEqualType []) then {
							private _overrideResult = [_target, _override, _result] call _processOverride;
							_override = _overrideResult select 0;
							_result = _overrideResult select 1;
						};

						private _type = _environment select 0;
						private _id = [] call KH_fnc_generateUid;

						switch _type do {
							case "AFTER_N_FRAMES": {
								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_frames", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_backupTarget", "_subfunction", "_override", "_id"];

									[
										{
											params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_backupTarget", "_subfunction", "_override", "_id"];
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState isNotEqualTo "TERMINATE") then {
												switch _idState do {
													case "ACTIVE": {																												
														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															[_backupArguments, _backupFunction, _backupTarget, _override] call _subfunction;
														};
													};

													case "INACTIVE": {
														[
															{
																private _id = _this select 5;
																private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																if (_idState isNotEqualTo "INACTIVE") then {
																	true;
																}
																else {
																	false;
																};
															}, 
															{
																params ["_backupArguments", "_backupFunction", "_backupTarget", "_override", "_subfunction", "_id"];
																private _id = _this select 5;
																private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																if (_idState == "ACTIVE") then {
																	[_backupArguments, _backupFunction, _backupTarget, _override] call _subfunction;
																};
															}, 
															[_backupArguments, _backupFunction, _backupTarget, _override, _subfunction, _id]
														] call CBA_fnc_waitUntilAndExecute;
													};
												};
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id],
										_frames
									] call CBA_fnc_execAfterNFrames;
								};

								private _frames = _environment select 1;
								private _conditionArguments = _environment param [2, []];
								private _conditionFunction = _environment param [3, {true;}];
								private _backupArguments = _environment param [4, []];
								private _backupFunction = _environment param [5, {}];
								private _backupTarget = _environment param [6, _target];
								_conditionFunction = [_conditionArguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _frames, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _frames, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id],
										_environmentCaller
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case "WAIT": {
								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_time", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_backupTarget", "_subfunction", "_override", "_id"];

									[
										{
											params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_backupTarget", "_subfunction", "_override", "_id"];
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState isNotEqualTo "TERMINATE") then {
												switch _idState do {
													case "ACTIVE": {
														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															[_backupArguments, _backupFunction, _backupTarget, _override] call _subfunction;
														};
													};

													case "INACTIVE": {
														[
															{
																private _id = _this select 5;
																private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																if (_idState isNotEqualTo "INACTIVE") then {
																	true;
																}
																else {
																	false;
																};
															}, 
															{
																params ["_backupArguments", "_backupFunction", "_backupTarget", "_override", "_subfunction", "_id"];
																private _id = _this select 5;
																private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																if (_idState == "ACTIVE") then {
																	[_backupArguments, _backupFunction, _backupTarget, _override] call _subfunction;
																};
															}, 
															[_backupArguments, _backupFunction, _backupTarget, _override, _subfunction, _id]
														] call CBA_fnc_waitUntilAndExecute;
													};
												};
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id],
										_time
									] call CBA_fnc_waitAndExecute;
								};

								private _time = _environment select 1;

								if (_time isEqualType "") then {
									_time = ((parseNumber _time) - CBA_missionTime) max 0;
								};

								private _conditionArguments = _environment param [2, []];
								private _conditionFunction = _environment param [3, {true;}];
								private _backupArguments = _environment param [4, []];
								private _backupFunction = _environment param [5, {}];
								private _backupTarget = _environment param [6, _target];
								_conditionFunction = [_conditionArguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _time, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _time, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _backupTarget, _subfunction, _override, _id],
										_environmentCaller
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case "WAIT_UNTIL": {
								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutArguments", "_timeoutFunction", "_timeoutTarget", "_subfunction", "_override", "_id"];

									[
										{
											private _id = _this select 10;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											switch _idState do {
												case "ACTIVE": {
													private _conditionArguments = _this select 3;
													private _conditionFunction = _this select 4;
													_conditionArguments call _conditionFunction;
												};

												case "INACTIVE": {
													false;
												};

												case "TERMINATE": {
													true;
												};
											};
										}, 
										{
											private _id = _this select 10;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState == "ACTIVE") then {
												private _arguments = _this select 0;
												private _function = _this select 1;
												private _target = _this select 2;
												private _subfunction = _this select 8;
												private _override = _this select 9;
												[_arguments, _function, _target, _override] call _subfunction;
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _id],
										_timeout,
										{
											private _id = _this select 10;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState == "ACTIVE") then {
												private _timeoutArguments = _this select 5;
												private _timeoutFunction = _this select 6;
												private _timeoutTarget = _this select 7;
												private _subfunction = _this select 8;
												private _override = _this select 9;
												[_timeoutArguments, _timeoutFunction, _timeoutTarget, _override] call _subfunction;
											};
										}
									] call CBA_fnc_waitUntilAndExecute;
								};

								private _conditionArguments = _environment select 1;
								private _conditionFunction = _environment select 2;
								private _timeout = _environment param [3, -1];
								private _timeoutArguments = _environment param [4, []];
								private _timeoutFunction = _environment param [5, {}];
								private _timeoutTarget = _environment param [6, _target];
								_conditionFunction = [_conditionArguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								if (_timeout isEqualType "") then {
									_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
								};

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _id] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _id],
										_environmentCaller
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case "INTERVAL": {
								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutOnConditionFailure", "_timeoutArguments", "_timeoutFunction", "_timeoutTarget", "_subfunction", "_override", "_interval", "_id"];

									private _handler = [
										{
											private _id = _args select 11;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];
											
											if (_idState isNotEqualTo "INACTIVE") then {
												switch _idState do {
													case "ACTIVE": {
														_args params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeoutOnConditionFailure", "_timeoutArguments", "_timeoutFunction", "_timeoutTarget", "_subfunction", "_override"];
														
														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															if _timeoutOnConditionFailure then {
																[_timeoutArguments, _timeoutFunction, _timeoutTarget, _override] call _subfunction;
																missionNamespace setVariable [_id, "TERMINATE"];
																[_handle] call CBA_fnc_removePerFrameHandler;
															};
														};
													};

													case "TERMINATE": {
														[_handle] call CBA_fnc_removePerFrameHandler;
													};		
												};
											};
										}, 
										_interval, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _id]
									] call CBA_fnc_addPerFrameHandler;

									if (_timeout != 0) then {
										[
											{
												params ["_timeoutArguments", "_timeoutFunction", "_timeoutTarget", "_subfunction", "_override", "_handler", "_id"];
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];
												
												if !(_idState == "TERMINATE") then {
													[_handler] call CBA_fnc_removePerFrameHandler;
													
													if (_idState == "ACTIVE") then {
														[_timeoutArguments, _timeoutFunction, _timeoutTarget, _override] call _subfunction;
													};
												};
											}, 
											[_timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _handler, _id], 
											_timeout
										] call CBA_fnc_waitAndExecute;
									};
								};

								private _interval = _environment select 1;
								private _conditionArguments = _environment param [2, []];
								private _conditionFunction = _environment param [3, {true;}];
								private _timeout = _environment param [4, 0];
								private _timeoutOnConditionFailure = _environment param [5, false];
								private _timeoutArguments = _environment param [6, []];
								private _timeoutFunction = _environment param [7, {}];
								private _timeoutTarget = _environment param [8, _target];
								_conditionFunction = [_conditionArguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								if (_timeout isEqualType "") then {
									_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
								};
								
								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _interval, _id] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _timeoutTarget, _subfunction, _override, _interval, _id],
										_environmentCaller
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case "SEQUENCE": {
								private _executions = _environment select 1;
								private _suspendInFrames = _environment param [2, false];
								private _overrides = [];
								private _results = [];
								private _newExecutions = [] call KH_fnc_generateUid;
								private _newExecutionEvent = [] call KH_fnc_generateUid;
								private _sequenceArguments = [] call KH_fnc_generateUid;

								{
									private _currentOverride = "";
									private _currentResult = true;
									private _currentTarget = (_x select 1) param [2, _target];

									if (_currentTarget isEqualType []) then {
										private _type = _currentTarget select 0;

										switch _type do {
											case "JIP": {
												if (_forEachIndex == 0) then {
													private _overrideId = _currentTarget param [5, ""];

													if (_overrideId != "") then {
														_currentOverride = _overrideId;
													}
													else {
														_currentOverride = _override;
													};

													_currentResult = ["JIP_HANDLER", _currentOverride];
												}
												else {
													private _overrideId = _currentTarget param [5, ""];

													if (_overrideId != "") then {
														_currentOverride = _overrideId;
													}
													else {
														_currentOverride = [] call KH_fnc_generateUid;
													};

													_currentResult = ["JIP_HANDLER", _currentOverride];
												};
											};

											case "PERSISTENT": {
												if (_forEachIndex == 0) then {
													_currentOverride = _override;
												}
												else {
													_currentOverride = [] call KH_fnc_generateUid;
												};

												_currentResult = ["PERSISTENT_HANDLER", _currentOverride, _target select 1];
											};
										};
									};

									_overrides pushBack _currentOverride;
									_results pushBack _currentResult;
								} forEach _executions;

								private _environmentCaller = {
									params ["_arguments", "_function", "_target", "_environment", "_executions", "_id", "_overrides", "_newExecutions", "_newExecutionEvent", "_suspendInFrames", "_sequenceArguments"];
									private _timeSuspensions = [0];
									private _conditionSuspensions = [[[], {true;}, true, -1, [], {}, _target, _environment, [] call KH_fnc_generateUid]];
									
									{
										(_x select 0) params [["_delay", 0], ["_conditionArguments", []], ["_conditionFunction", {true;}], ["_timeout", -1], ["_timeoutArguments", []], ["_timeoutFunction", {}], ["_timeoutTarget", _target], ["_timeoutEnvironment", _environment]];
										(_x select 1) params [["_currentArguments", _arguments], ["_currentFunction", _function], ["_currentTarget", _target], ["_currentEnvironment", _environment]];

										if ((_delay isEqualType "") && !_suspendInFrames) then {
											_delay = ((parseNumber _delay) - CBA_missionTime) max 0;
										};

										_timeSuspensions pushBack _delay;
										_conditionSuspensions pushBack [_conditionArguments, [_conditionArguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction, _evaluatePrevious, _timeout, _timeoutArguments, _timeoutFunction, _timeoutTarget, _timeoutEnvironment, [] call KH_fnc_generateUid];
										private _currentExecutionId = [] call KH_fnc_generateUid;
										private _conditionId = [] call KH_fnc_generateUid;

										[
											{
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];
												private _currentExecutionId = _this select 12;
												private _condition = true;

												switch _idState do {
													case "ACTIVE": {
														private _conditionSuspensions = _this select 6;
														private _conditionId = _this select 7;
														private _sequenceArguments = _this select 13;

														if ((_conditionSuspensions select -1) select 2) then {												
															{
																_x params ["_conditionArguments", "_conditionFunction"];

																private _expression = [
																	"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
																	call ", _conditionFunction, ";"
																] joinString "";

																if !(_conditionArguments call (compile _expression)) then {
																	if !(_x in (missionNamespace getVariable [_conditionId, []])) then {
																		_condition = false;
																		break;
																	};
																}
																else {
																	if ((_conditionSuspensions select -1) isNotEqualTo _x) then {
																		private _currentConditions = missionNamespace getVariable [_conditionId, []];
																		_currentConditions pushBack _x;
																		missionNamespace setVariable [_conditionId, _currentConditions];
																	};
																};
															} forEach _conditionSuspensions;
														}
														else {
															(_conditionSuspensions select -1) params ["_conditionArguments", "_conditionFunction"];

															private _expression = [
																"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
																call ", _conditionFunction, ";"
															] joinString "";

															if !(_conditionArguments call (compile _expression)) then {
																_condition = false;
															};
														};
													};

													case "INACTIVE": {
														_condition = false;
													};

													case "TERMINATE": {
														_condition = true;
													};		
												};

												if !(missionNamespace getVariable [_currentExecutionId, false]) then {
													_condition = false;
												};

												_condition;
											}, 
											{
												params ["_currentArguments", "_currentFunction", "_currentTarget", "_currentEnvironment", "_environment", "_subfunction", "_conditionSuspensions"];
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState == "ACTIVE") then {
													private _overrides = _this select 9;
													private _newExecutions = _this select 10;
													private _newExecutionEvent = _this select 11;
													private _sequenceArguments = _this select 13;
													_currentFunction = [_currentArguments, [_currentFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

													private _expression = [
														"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
														private _sequenceResult = call ", _currentFunction, ";
														missionNamespace setVariable ['", _sequenceArguments, "', _sequenceResult];"
													] joinString "";
												
													if (_currentEnvironment isEqualTo _environment) then {
														[_currentArguments, compile _expression, _currentTarget, _overrides select ((count _conditionSuspensions) - 2)] call _subfunction;
													}
													else {
														private _currentExecutions = missionNamespace getVariable [_newExecutions, []];
														private _newExecution = [_currentArguments, compile _expression, _currentTarget, _currentEnvironment] call KH_fnc_execute;
														_currentExecutions pushBack _newExecution;
														missionNamespace setVariable [_newExecutions, _currentExecutions];
														[_newExecutionEvent, [_newExecution, _currentExecutions, missionNamespace getVariable [_sequenceArguments, []]]] call CBA_fnc_localEvent;
													};
												};
											}, 
											[_currentArguments, _currentFunction, _currentTarget, _currentEnvironment, _environment, _subfunction, _conditionSuspensions, _conditionId, _id, _overrides, _newExecutions, _newExecutionEvent, _currentExecutionId, _sequenceArguments],
											(_conditionSuspensions select -1) select 3,
											{
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState == "ACTIVE") then {
													private _environment = _this select 4;
													private _subfunction = _this select 5;
													private _overrides = _this select 9;
													private _newExecutions = _this select 10;
													private _newExecutionEvent = _this select 11;
													private _sequenceArguments = _this select 13;
													private _timeoutArguments = ((_this select 6) select -1) select 4;
													private _timeoutFunction = ((_this select 6) select -1) select 5;
													private _timeoutTarget = ((_this select 6) select -1) select 6;
													private _timeoutEnvironment = ((_this select 6) select -1) select 7;
													_timeoutFunction = [_timeoutArguments, [_timeoutFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

													private _expression = [
														"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
														private _sequenceResult = call ", _timeoutFunction, ";
														missionNamespace setVariable ['", _sequenceArguments, "', _sequenceResult];"
													] joinString "";

													if (_timeoutEnvironment isEqualTo _environment) then {
														[_timeoutArguments, compile _expression, _timeoutTarget, _overrides select ((count _conditionSuspensions) - 2)] call _subfunction;
													}
													else {
														private _currentExecutions = missionNamespace getVariable [_newExecutions, []];
														private _newExecution = [_timeoutArguments, compile _expression, _timeoutTarget, _timeoutEnvironment] call KH_fnc_execute;
														_currentExecutions pushBack _newExecution;
														missionNamespace setVariable [_newExecutions, _currentExecutions];
														[_newExecutionEvent, [_newExecution, _currentExecutions, missionNamespace getVariable [_sequenceArguments, []]]] call CBA_fnc_localEvent;
													};
												};
											}
										] call CBA_fnc_waitUntilAndExecute;

										if _suspendInFrames then {
											[
												{
													params ["_currentExecutionId"];
													missionNamespace setVariable [_currentExecutionId, true];
												}, 
												[_currentExecutionId],
												[_timeSuspensions] call KH_fnc_arraySum
											] call CBA_fnc_execAfterNFrames;
										}
										else {
											[
												{
													params ["_currentExecutionId"];
													missionNamespace setVariable [_currentExecutionId, true];
												}, 
												[_currentExecutionId],
												[_timeSuspensions] call KH_fnc_arraySum
											] call CBA_fnc_waitAndExecute;
										};
									} forEach _executions;
								};

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, "THIS_FRAME", _executions, _id, _overrides, _newExecutions, _newExecutionEvent, _suspendInFrames, _sequenceArguments] call _environmentCaller;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, "THIS_FRAME", _executions, _id, _overrides, _newExecutions, _newExecutionEvent, _suspendInFrames, _sequenceArguments],
										_environmentCaller
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _results, _newExecutions, _newExecutionEvent, _sequenceArguments];
							};

							default {
								false;
							};
						};
					};

					default {
						false;
					};
				};
			}
			else {
				false;	
			};
		}
	) select 0;
};

if !(isNil "_return") then {
	_return;
}
else {
	false;
};