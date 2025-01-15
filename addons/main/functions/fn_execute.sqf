private "_return";

isNil {
	_return = (
		[_x] apply {
			if (((((count _this) > 2) && !(isNil {param [2, nil]})) || (((count _this) < 3))) && !(isNil {param [1, nil]}) && !(isNil {param [0, nil]})) then {
				params ["_arguments", "_function", ["_target", "GLOBAL"], ["_environment", "THIS_FRAME"]];

				if ((!(_function isEqualType {}) && !(_function isEqualType "")) || !(_arguments isEqualType [])) exitWith {
					false;
				};

				private _subfunction = {
					params ["_arguments", "_function", "_target", "_override"];

					private _functionProcessor = {
						params ["_arguments", "_function"];

						if (_function isEqualType "") then {
							private _parsedFunction = missionNamespace getVariable [_function, {}];

							if (_parsedFunction isEqualTo {}) then {
								switch true do {
									case ((count _arguments) == 0): {
										[] call (compile ([_function] joinString ""));
									};

									case ((count _arguments) == 1): {
										private _unaryArgument = [missionNamespace, "KH_var_unaryArgument", _arguments select 0, false] call KH_fnc_atomicVariable;
										[] call (compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString ""));
									};

									case ((count _arguments) == 2): {
										private _binaryArguments = [missionNamespace, "KH_var_binaryArguments", _arguments, false] call KH_fnc_atomicVariable;
										[] call (compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString ""));
									};
								};
							}
							else {
								_arguments call _parsedFunction;
							};
						}
						else {
							_arguments call _function;
						};
					};
					
					switch true do {
						case (_target isEqualType objNull): {
							if !(isNull _target) then {
								if (local _target) then {
									[_arguments, _function] call _functionProcessor;
								}
								else {
									["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_targetEvent;
								};

								true;
							}
							else {
								false;
							};
						};

						case (_target isEqualType teamMemberNull): {
							if !(isNull _target) then {
								if (local (agent _target)) then {
									[_arguments, _function] call _functionProcessor;
								}
								else {
									["KH_eve_executionGlobal", [_arguments, _function], agent _target] call CBA_fnc_targetEvent;
								};

								true;
							}
							else {
								false;
							};
						};

						case (_target isEqualType grpNull): {
							if !(isNull _target) then {
								if (local _target) then {
									[_arguments, _function] call _functionProcessor;
								}
								else {
									[
										"KH_eve_executionServer", 
										[
											[_arguments, _function, _target],
											{
												params ["_arguments", "_function", "_target"];
												["KH_eve_executionGlobal", [_arguments, _function], groupOwner _target] call CBA_fnc_ownerEvent;
											}
										]
									] call CBA_fnc_serverEvent;
								};

								true;
							}
							else {
								false;
							};
						};
						
						case (_target isEqualType 0): {
							if (_target == clientOwner) then {
								[_arguments, _function] call _functionProcessor;
								true;
							}
							else {
								if !(_target < 0) then {
									if (_target in KH_var_allMachines) then {
										["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_ownerEvent;
										true;
									}
									else {
										false;
									};
								}
								else {
									private _invertedTarget = abs _target;

									if (_invertedTarget == clientOwner) then {
										["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_remoteEvent;
										true;
									}
									else {
										if (_invertedTarget in KH_var_allMachines) then {
											{
												["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
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

						case (_target isEqualType sideUnknown): {
							["KH_eve_executionPlayer", [_arguments, _function], [[_target], true] call KH_fnc_getClients] call CBA_fnc_targetEvent;
							true;
						};

						case (_target isEqualType ""): {
							switch true do {
								case (_target == "SERVER"): {
									if isServer then {
										[_arguments, _function] call _functionProcessor;
									}
									else {
										["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
									};

									true;
								};

								case (_target == "GLOBAL"): {
									["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_globalEvent;
									true;
								};

								case (_target == "PLAYERS"): {
									["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
									true;
								};

								case (_target == "PLAYER_UNITS"): {
									{
										["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case (_target == "HEADLESS"): {
									["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_globalEvent;
									true;
								};

								case (_target == "CURATORS"): {
									{
										private _curatorUnit = getAssignedCuratorUnit _x;

										if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
											["KH_eve_executionPlayer", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
										};
									} forEach allCurators;

									true;
								};

								case (_target == "ADMIN"): {
									if (KH_var_currentAdmin != 2) then {
										if (KH_var_currentAdmin == clientOwner) then {
											[_arguments, _function] call _functionProcessor;
										}
										else {
											["KH_eve_executionPlayer", [_arguments, _function], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
										};

										true;
									}
									else {
										false;
									};
								};

								case (((parseNumber (_target select [0, 1])) != 0) && ([_target] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
									if !(":" in _target) then {
										if ((count _target) == 17) then {
											private _player = KH_var_allPlayerUidMachines get _target;

											if !(isNil "_player") then {
												["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
												true;
											}
											else {
												false;
											};
										}
										else {
											private _player = KH_var_allPlayerIdMachines get _target;

											if !(isNil "_player") then {
												["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
												true;
											}
											else {
												private _client = KH_var_allHeadlessIdMachines get _target;

												if !(isNil "_client") then {
													["KH_eve_executionHeadless", [_arguments, _function], _client] call CBA_fnc_ownerEvent;
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
												[_arguments, _function] call _functionProcessor;
											}
											else {
												["KH_eve_executionGlobal", [_arguments, _function], objectFromNetId _target] call CBA_fnc_targetEvent;
											};

											true;
										}
										else {
											if !(isNull (groupFromNetId _target)) then {
												if (local (groupFromNetId _target)) then {
													[_arguments, _function] call _functionProcessor;
												}
												else {
													[
														"KH_eve_executionServer", 
														[
															[_arguments, _function, _target],
															{
																params ["_arguments", "_function", "_target"];
																["KH_eve_executionGlobal", [_arguments, _function], groupOwner (groupFromNetId _target)] call CBA_fnc_ownerEvent;
															}
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
								};

								case (_target == "LOCAL"): {
									[_arguments, _function] call _functionProcessor;
								};
								
								default {
									private _unitPresent = false;

									{
										if ((name _x) == _target) then {
											["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											_unitPresent = true;
											break;
										};
									} forEach KH_var_allPlayerUnits;
									
									_unitPresent;
								};
							};
						};
						
						case (_target isEqualType []): {
							private _type = _target select 0;

							switch true do {
								case (_type == "GROUP_PLAYERS"): {
									private _group = _target select 1;
									private _invert = _target param [2, false];

									if !(isNull _group) then {
										{
											if !_invert then {
												if ((group _x) == _group) then {
													["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
												};
											}
											else {
												if ((group _x) != _group) then {
													["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
												};
											};
										} forEach KH_var_allPlayerUnits;

										true;
									}
									else {
										false;
									};
								};

								case (_type == "PLAYER_ROLE"): {
									private _role = _target select 1;
									private _invert = _target param [2, false];
									
									{
										if !_invert then {
											if ((roleDescription _x) == _role) then {
												["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											};
										}
										else {
											if ((roleDescription _x) != _role) then {
												["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											};
										};
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case (_type == "AREA_PLAYERS"): {
									private _area = _target select 1;
									private _invert = _target param [2, false];
									
									{
										if _invert then {
											if !(_x inArea _area) then {
												["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											};
										}
										else {
											if (_x inArea _area) then {
												["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											};
										};
									} forEach KH_var_allPlayerUnits;

									true;
								};

								case (_type == "CONDITION"): {
									private _conditionArguments = _target select 1;
									private _conditionFunction = _target select 2;
									private _exclusiveType = _target param [3, "GLOBAL"];
									private _eventType = "";

									switch true do {
										case (_exclusiveType == "GLOBAL"): {
											_eventType = "KH_eve_executionGlobal";
										};

										case (_exclusiveType == "SERVER"): {
											_eventType = "KH_eve_executionServer";
										};

										case (_exclusiveType == "PLAYERS"): {
											_eventType = "KH_eve_executionPlayer";
										};

										case (_exclusiveType == "HEADLESS"): {
											_eventType = "KH_eve_executionHeadless";
										};
									};

									[
										_eventType, 
										[
											[_arguments, _function, _conditionArguments, _conditionFunction, _functionProcessor], 
											{
												params ["_arguments", "_function", "_conditionArguments", "_conditionFunction", "_functionProcessor"];

												if (_conditionFunction isEqualType "") then {
													_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
												};
												
												if (_conditionArguments call _conditionFunction) then {
													[_arguments, _function] call _functionProcessor;
												};
											}
										]
									] call CBA_fnc_globalEvent;

									true;
								};

								case (_type == "PERFORMANT_MACHINE"): {
									private _playersOnly = _target param [1, false];
									private _invert = _target param [2, false];
									private _framerates = [missionNamespace, "KH_var_machineFramerates", createHashMap, false] call KH_fnc_atomicVariable;
									private _id = format ["KH_eve_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
									private _machines = [];

									private _callbackHandler = [
										"CBA",
										_id,
										[_framerates],
										{
											_args params ["_framerates"];
											private _framerateList = missionNamespace getVariable [_framerates, createHashMap];
											_framerateList insert [_this];
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

													[_id, [_framerate, clientOwner], _clientOwner] call CBA_fnc_ownerEvent;
												}
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

										["KH_eve_executionGlobal", [_arguments, _function], _chosenMachine] call CBA_fnc_ownerEvent;
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

								case (_type == "REMOTE"): {
									private _exclusiveType = _target param [1, "GLOBAL"];
									
									switch true do {
										case (_exclusiveType isEqualType ""): {
											switch true do {
												case (_exclusiveType == "GLOBAL"): {
													["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_remoteEvent;
												};

												case (_exclusiveType == "PLAYERS"): {
													["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_remoteEvent;
												};

												case (_exclusiveType == "PLAYER_UNITS"): {
													{
														["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
													} forEach (KH_var_allPlayerUnits - [player]);
												};

												case (_exclusiveType == "CURATORS"): {
													{
														private _curatorUnit = getAssignedCuratorUnit _x;

														if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
															["KH_eve_executionPlayer", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
														};
													} forEach (allCurators - [getAssignedCuratorLogic player]);
												};

												case (_exclusiveType == "HEADLESS"): {
													["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_remoteEvent;
												};
											};

											true;
										};

										case (_exclusiveType isEqualType []): {
											private _targetType = _exclusiveType select 0;

											switch true do {
												case (_targetType == "GROUP_PLAYERS"): {
													private _group = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];

													if !(isNull _group) then {
														{
															if !_invert then {
																if ((group _x) == _group) then {
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																};
															}
															else {
																if ((group _x) != _group) then {
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																};
															};
														} forEach (KH_var_allPlayerUnits - [player]);

														true;
													}
													else {
														false;
													};
												};

												case (_targetType == "PLAYER_ROLE"): {
													private _role = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];
													
													{
														if !_invert then {
															if ((roleDescription _x) == _role) then {
																["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
															};
														}
														else {
															if ((roleDescription _x) != _role) then {
																["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
															};
														};
													} forEach (KH_var_allPlayerUnits - [player]);

													true;
												};

												case (_targetType == "AREA_PLAYERS"): {
													private _area = _exclusiveType select 1;
													private _invert = _exclusiveType param [2, false];
													
													{
														if _invert then {
															if !(_x inArea _area) then {
																["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
															};
														}
														else {
															if (_x inArea _area) then {
																["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
															};
														};
													} forEach (KH_var_allPlayerUnits - [player]);

													true;
												};

												case (_targetType == "CONDITION"): {
													private _conditionArguments = _exclusiveType select 1;
													private _conditionFunction = _exclusiveType select 2;
													private _exclusiveType = _exclusiveType param [3, "GLOBAL"];
													private _eventType = "";

													switch true do {
														case (_exclusiveType == "GLOBAL"): {
															_eventType = "KH_eve_executionGlobal";
														};

														case (_exclusiveType == "SERVER"): {
															_eventType = "KH_eve_executionServer";
														};

														case (_exclusiveType == "PLAYERS"): {
															_eventType = "KH_eve_executionPlayer";
														};

														case (_exclusiveType == "HEADLESS"): {
															_eventType = "KH_eve_executionHeadless";
														};
													};

													[
														_eventType, 
														[
															[_arguments, _function, _conditionArguments, _conditionFunction, _functionProcessor], 
															{
																params ["_arguments", "_function", "_conditionArguments", "_conditionFunction", "_functionProcessor"];

																if (_conditionFunction isEqualType "") then {
																	_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
																};
																
																if (_conditionArguments call _conditionFunction) then {
																	[_arguments, _function] call _functionProcessor;
																};
															}
														]
													] call CBA_fnc_remoteEvent;

													true;
												};
											};											
										};
									};
								};

								case (_type == "TARGETS"): {
									private _targets = _target select 1;

									[
										"KH_eve_executionServer", 
										[
											[_arguments, _function, _targets, _functionProcessor, clientOwner],
											{
												params ["_arguments", "_function", "_targets", "_functionProcessor", "_caller"];
												private _exclusiveList = [];

												{
													switch true do {
														case (_x isEqualType objNull): {
															if !(isNull _x) then {
																if !((owner _x) in _exclusiveList) then {
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																	_exclusiveList pushBack (owner _x);
																};
															};
														};

														case (_x isEqualType teamMemberNull): {
															if !(isNull _x) then {
																private _agent = agent _x;

																if !((owner _agent) in _exclusiveList) then {
																	["KH_eve_executionGlobal", [_arguments, _function], _agent] call CBA_fnc_targetEvent;
																	_exclusiveList pushBack (owner _agent);
																};
															};
														};

														case (_x isEqualType grpNull): {															
															if !(isNull _x) then {
																if !((groupOwner _x) in _exclusiveList) then {
																	["KH_eve_executionGlobal", [_arguments, _function], groupOwner _x] call CBA_fnc_ownerEvent;
																	_exclusiveList pushBack (groupOwner _x);
																};
															};
														};
														
														case (_x isEqualType 0): {
															if (_x in KH_var_allMachines) then {
																if !(_x in _exclusiveList) then {
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	_exclusiveList pushBack _x;
																};
															};
														};

														case (_x isEqualType sideUnknown): {
															private _side = _x;

															{
																if !((owner _x) in _exclusiveList) then {
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																	_exclusiveList pushBack (owner _x);
																};
															} forEach ([[_side], true] call KH_fnc_getClients);
														};
														
														case (_x isEqualType ""): {
															switch true do {
																case (_x == "SERVER"): {
																	if !(2 in _exclusiveList) then {
																		[_arguments, _function] call _functionProcessor;
																		_exclusiveList pushBack 2;
																	};
																};

																case (_x == "PLAYERS"): {
																	{
																		if !(_x in _exclusiveList) then {
																			["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																			_exclusiveList pushBack _x;
																		};
																	} forEach KH_var_allPlayerMachines;
																};

																case (_x == "PLAYER_UNITS"): {
																	{
																		if !((owner _x) in _exclusiveList) then {
																			["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																			_exclusiveList pushBack (owner _x);
																		};
																	} forEach KH_var_allPlayerUnits;
																};

																case (_x == "HEADLESS"): {
																	{
																		if !(_x in _exclusiveList) then {
																			["KH_eve_executionHeadless", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																			_exclusiveList pushBack _x;
																		};
																	} forEach KH_var_allHeadlessMachines;
																};

																case (_x == "CURATORS"): {
																	{
																		private _curatorUnit = getAssignedCuratorUnit _x;

																		if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
																			if !((owner _curatorUnit) in _exclusiveList) then {
																				["KH_eve_executionPlayer", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
																				_exclusiveList pushBack (owner _curatorUnit);
																			};
																		};
																	} forEach allCurators;
																};

																case (_x == "ADMIN"): {
																	if (KH_var_currentAdmin != 2) then {
																		if !(KH_var_currentAdmin in _exclusiveList) then {
																			["KH_eve_executionPlayer", [_arguments, _function], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
																			_exclusiveList pushBack KH_var_currentAdmin;
																		};
																	};
																};

																case (((parseNumber (_x select [0, 1])) != 0) && ([_x] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
																	if !(":" in _x) then {
																		if ((count _x) == 17) then {
																			private _player = KH_var_allPlayerUidMachines get _x;

																			if !(isNil "_player") then {
																				if !(_player in _exclusiveList) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
																					_exclusiveList pushBack _player;
																				};
																			};
																		}
																		else {
																			private _player = KH_var_allPlayerIdMachines get _x;

																			if !(isNil "_player") then {
																				if !(_player in _exclusiveList) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
																					_exclusiveList pushBack _player;
																				};
																			}
																			else {
																				private _client = KH_var_allHeadlessIdMachines get _x;

																				if !(isNil "_client") then {
																					if !(_client in _exclusiveList) then {
																						["KH_eve_executionHeadless", [_arguments, _function], _client] call CBA_fnc_ownerEvent;
																						_exclusiveList pushBack _client;
																					};
																				};
																			};
																		};
																	}
																	else {
																		if !(isNull (objectFromNetId _x)) then {
																			["KH_eve_executionGlobal", [_arguments, _function], objectFromNetId _x] call CBA_fnc_targetEvent;
																			_exclusiveList pushBack (owner (objectFromNetId _x));
																		}
																		else {
																			if !(isNull (groupFromNetId _x)) then {
																				[
																					"KH_eve_executionServer", 
																					[
																						[_arguments, _function, _x],
																						{
																							params ["_arguments", "_function", "_target"];
																							["KH_eve_executionGlobal", [_arguments, _function], groupOwner (groupFromNetId _target)] call CBA_fnc_ownerEvent;
																						}
																					]
																				] call CBA_fnc_serverEvent;

																				_exclusiveList pushBack (groupOwner (groupFromNetId _x));
																			};											
																		};
																	};
																};

																case (_x == "LOCAL"): {
																	if !(_caller in _exclusiveList) then {
																		["KH_eve_executionGlobal", [_arguments, _function], _caller] call CBA_fnc_ownerEvent;
																		_exclusiveList pushBack _caller;
																	};
																};

																default {
																	private _currentTarget = _x;

																	{
																		if ((name _x) == _currentTarget) then {
																			if !((owner _x) in _exclusiveList) then {
																				["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																				_exclusiveList pushBack (owner _x);
																			};

																			break;
																		};
																	} forEach KH_var_allPlayerUnits;
																};
															};
														};

														case (_x isEqualType []): {
															private _currentTarget = _x;
															private _targetType = _currentTarget select 0;

															switch true do {
																case (_targetType == "GROUP_PLAYERS"): {
																	private _group = _currentTarget select 1;
																	private _invert = _currentTarget param [2, false];
																
																	if !(isNull _group) then {
																		{
																			if !((owner _x) in _exclusiveList) then {
																				if !_invert then {
																					if ((group _x) == _group) then {
																						["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																					};
																				}
																				else {
																					if ((group _x) != _group) then {
																						["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																					};
																				};

																				_exclusiveList pushBack (owner _x);
																			};
																		} forEach KH_var_allPlayerUnits;
																	};
																};

																case (_targetType == "PLAYER_ROLE"): {
																	private _role = _currentTarget select 1;
																	private _invert = _currentTarget param [2, false];
																	
																	{
																		if !((owner _x) in _exclusiveList) then {
																			if !_invert then {
																				if ((roleDescription _x) == _role) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																				};
																			}
																			else {
																				if ((roleDescription _x) != _role) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																				};
																			};

																			_exclusiveList pushBack (owner _x);
																		};
																	} forEach KH_var_allPlayerUnits;
																};

																case (_targetType == "AREA_PLAYERS"): {
																	private _area = _currentTarget select 1;
																	private _invert = _currentTarget param [2, false];
																	
																	{
																		if !((owner _x) in _exclusiveList) then {
																			if _invert then {
																				if !(_x inArea _area) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																					_exclusiveList pushBack (owner _x);
																				};
																			}
																			else {
																				if (_x inArea _area) then {
																					["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																					_exclusiveList pushBack (owner _x);
																				};
																			};
																		};
																	} forEach KH_var_allPlayerUnits;
																};
															};
														};
													};
												} forEach _targets;
											}
										]
									] call CBA_fnc_serverEvent;

									true;
								};

								case (_type == "JIP"): {
									private _exclusiveType = _target param [1, "GLOBAL"];
									private _dependency = _target param [2, true];
									private _unitRequired = _target param [3, true];
									private _remote = _target param [4, false];
									private _overrideId = _target param [5, ""];
									private _id = "";
									private _jipOverrideParams = "";
									private _jipOverrideId = "";

									if (_overrideId != "") then {
										_id = _overrideId;
										_jipOverrideParams = format ["KH_var_jipOverrideParams%1", _overrideId];
										_jipOverrideId = format ["KH_var_jipOverrideId%1", _overrideId];
									}
									else {
										if (_override != "") then {
											_id = _override;
										}
										else {
											_id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
										};
									};

									[
										"KH_eve_executionServer", 
										[
											[_arguments, _function, _exclusiveType, _dependency, _unitRequired, _remote, _id, _jipOverrideParams, _jipOverrideId, clientOwner], 
											{
												params ["_arguments", "_function", "_exclusiveType", "_dependency", "_unitRequired", "_remote", "_id", "_jipOverrideParams", "_jipOverrideId", "_originalOwner"];

												switch true do {
													case (_exclusiveType == "GLOBAL"): {
														if !_remote then {
															if !_unitRequired then {
																{
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																} forEach KH_var_allMachines;
															}
															else {
																{
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																} forEach KH_var_allPlayerUnits;
																
																if isDedicated then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach ([2] + KH_var_allHeadlessMachines);
																}
																else {
																	{
																		["KH_eve_executionHeadless", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach KH_var_allHeadlessMachines;
																};
															};
														}
														else {
															if !_unitRequired then {
																{
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																} forEach (KH_var_allMachines - [_originalOwner]);
															}
															else {
																{
																	if ((owner _x) != _originalOwner) then {
																		["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																	};
																} forEach KH_var_allPlayerUnits;

																if isDedicated then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach (([2] + KH_var_allHeadlessMachines) - [_originalOwner]);
																}
																else {
																	{
																		["KH_eve_executionHeadless", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach (KH_var_allHeadlessMachines - [_originalOwner]);
																};
															};
														};
													};

													case (_exclusiveType == "PLAYERS"): {
														if !_remote then {
															if !_unitRequired then {
																{
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																} forEach KH_var_allPlayerMachines;
															}
															else {
																{
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																} forEach KH_var_allPlayerUnits;
															};
														}
														else {
															if !_unitRequired then {
																{
																	["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																} forEach (KH_var_allPlayerMachines - [_originalOwner]);
															}
															else {
																{
																	if ((owner _x) != _originalOwner) then {
																		["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																	};
																} forEach KH_var_allPlayerUnits;
															};
														};
													};

													case (_exclusiveType == "HEADLESS"): {
														if !_remote then {
															{
																["KH_eve_executionHeadless", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
															} forEach KH_var_allHeadlessMachines;
														}
														else {
															{
																["KH_eve_executionHeadless", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
															} forEach (KH_var_allHeadlessMachines - [_originalOwner]);
														};
													};
												};

												private _joinType = [];
												
												if _unitRequired then {
													switch true do {
														case ((_exclusiveType == "GLOBAL") || (_exclusiveType == "JIP_GLOBAL")): {
															_joinType = ["KH_eve_playerLoaded", "KH_eve_headlessPreloaded"];
														};

														case ((_exclusiveType == "PLAYERS") || (_exclusiveType == "JIP_PLAYERS")): {
															_joinType = ["KH_eve_playerLoaded"];
														};

														case ((_exclusiveType == "HEADLESS") || (_exclusiveType == "JIP_HEADLESS")): {
															_joinType = ["KH_eve_headlessPreloaded"];
														};
													};
												}
												else {
													switch true do {
														case ((_exclusiveType == "GLOBAL") || (_exclusiveType == "JIP_GLOBAL")): {
															_joinType = ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
														};

														case ((_exclusiveType == "PLAYERS") || (_exclusiveType == "JIP_PLAYERS")): {
															_joinType = ["KH_eve_playerPreloadedInitial"];
														};

														case ((_exclusiveType == "HEADLESS") || (_exclusiveType == "JIP_HEADLESS")): {
															_joinType = ["KH_eve_headlessPreloaded"];
														};
													};
												};

												if (_jipOverrideId == "") then {
													_jipOverrideId = [missionNamespace, "KH_var_jipOverrideIdDefault", false, false] call KH_fnc_atomicVariable;
												};
												
												if !(missionNamespace getVariable [_jipOverrideId, false]) then {
													{
														[
															"CBA",
															_x,
															[_arguments, _function, _dependency, _id, _jipOverrideParams, _x],
															{
																_args params ["_arguments", "_function", "_dependency", "_id", "_jipOverrideParams", "_joinType"];
																private _joiningMachine = 999999;

																if (_joinType == "KH_eve_playerLoaded") then {
																	_joiningMachine = _this select 1;
																}
																else {
																	_joiningMachine = _this select 0;
																};

																if (_jipOverrideParams != "") then {
																	_arguments = (missionNamespace getVariable [_jipOverrideParams, []]) select 0;
																	_function = (missionNamespace getVariable [_jipOverrideParams, {}]) select 1;
																	_dependency = (missionNamespace getVariable [_jipOverrideParams, true]) select 2;
																};
																
																switch true do {
																	case (_dependency isEqualType ""): {
																		if ((parseNumber (_dependency select [0, 1])) != 0) then {
																			private _uid = "";

																			if _joinType == "KH_eve_playerLoaded" then {
																				_uid = getPlayerUID (_this select 0); 
																			}
																			else {
																				_uid = _this select 1;
																			};

																			if (_dependency == _uid) then {
																				if !(_idState == "INACTIVE") then {
																					switch true do {
																						case (_idState == "ACTIVE"): {
																							["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																						};

																						case (_idState == "TERMINATE"): {
																							[_eventName, _localId] call CBA_fnc_removeEventHandler;
																						};		
																					};
																				};
																			}
																			else {
																				if (":" in _dependency) then {
																					if (!(isNull (objectFromNetId _dependency)) || !(isNull (groupFromNetId _dependency))) then {
																						if !(_idState == "INACTIVE") then {
																							switch true do {
																								case (_idState == "ACTIVE"): {
																									["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																								};

																								case (_idState == "TERMINATE"): {
																									[_eventName, _localId] call CBA_fnc_removeEventHandler;
																								};		
																							};
																						};
																					};
																				}
																				else {
																					private _player = KH_var_allPlayerIdMachines get _x;
																					private _client = KH_var_allHeadlessIdMachines get _x;

																					if (!(isNil "_player") || !(isNil "_client")) then {
																						if !(_idState == "INACTIVE") then {
																							switch true do {
																								case (_idState == "ACTIVE"): {
																									["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																								};

																								case (_idState == "TERMINATE"): {
																									[_eventName, _localId] call CBA_fnc_removeEventHandler;
																								};		
																							};
																						};
																					};
																				};
																			};
																		}
																		else {
																			if (missionNamespace getVariable [_dependency, false]) then {
																				private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																				if !(_idState == "INACTIVE") then {
																					switch true do {
																						case (_idState == "ACTIVE"): {
																							["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																						};

																						case (_idState == "TERMINATE"): {
																							[_eventName, _localId] call CBA_fnc_removeEventHandler;
																						};		
																					};
																				};
																			};
																		};
																	};

																	case ((_dependency isEqualType objNull) || (_dependency isEqualType teamMemberNull) || (_dependency isEqualType grpNull)): {
																		if !(isNull _dependency) then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_eventName, _localId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		};
																	};

																	case (_dependency isEqualType 0): {
																		if (_dependency in KH_var_allMachines) then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_eventName, _localId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		};
																	};

																	case (_dependency isEqualType {}): {
																		if ([] call _dependency) then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_eventName, _localId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		};
																	};

																	case (_dependency isEqualType []): {
																		private _condition = true;
																		
																		{
																			switch true do {
																				case (_x isEqualType ""): {
																					if ((parseNumber (_x select [0, 1])) != 0) then {
																						private _uid = "";

																						if _joinType == "KH_eve_playerLoaded" then {
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

																				case ((_x isEqualType objNull) || (_x isEqualType teamMemberNull) || (_x isEqualType grpNull)): {
																					if (isNull _x) then {
																						_condition = false;
																						break;
																					};
																				};

																				case (_x isEqualType 0): {
																					if !(_x in KH_var_allMachines) then {
																						_condition = false;
																						break;
																					};
																				};

																				case (_x isEqualType {}): {
																					if !([] call _x) then {
																						_condition = false;
																						break;
																					};
																				};
																			};
																		} forEach _dependency;

																		if _condition then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];
																			
																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_eventName, _localId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		};
																	};

																	case (_dependency isEqualType true): {
																		if _dependency then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_eventName, _localId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		};
																	};
																};
															}
														] call KH_fnc_addEventHandler;
													} forEach _joinType;
												};

												missionNamespace setVariable [_jipOverrideId, true];

												if (_jipOverrideParams != "") then {
													missionNamespace setVariable [_jipOverrideParams, [_arguments, _function, _dependency]];
												};
											}
										]
									] call CBA_fnc_serverEvent;

									["JIP_HANDLER", _id];
								};

								case (_type == "CALLBACK"): {
									private _callbackTarget = _target select 1;
									private _callbackArguments = _target select 2;
									private _callbackFunction = _target select 3;
									private _id = format ["KH_eve_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];

									[
										"CBA",
										_id,
										[_arguments, _function],
										{
											_args params ["_arguments", "_function"];
											private _argsCallback = [missionNamespace, "KH_var_callbackHandlerArguments", _this, false] call KH_fnc_atomicVariable;

											if (_function isEqualType "") then {
												_function = (missionNamespace getVariable [_function, {}]);
											};

											private _expression = [
												"private _argsCallback = (missionNamespace getVariable ['", _argsCallback, "', []]);
												call ", _function, ";"
											] joinString "";

											_arguments call (compile _expression);
											[_eventName, _localId] call CBA_fnc_removeEventHandler;
										}
									] call KH_fnc_addEventHandler;

									private _callbackSendFunction = {
										params ["_arguments", "_function", "_id", "_owner"];

										if (_function isEqualType "") then {
											private _parsedFunction = missionNamespace getVariable [_function, {}];

											if (_parsedFunction isEqualTo {}) then {
												switch true do {
													case ((count _arguments) == 0): {
														[_id, [[] call (compile ([_function] joinString ""))], _owner] call CBA_fnc_ownerEvent;
													};

													case ((count _arguments) == 1): {
														private _unaryArgument = [missionNamespace, "KH_var_unaryArgument", _arguments select 0, false] call KH_fnc_atomicVariable;
														[_id, [[] call (compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString ""))], _owner] call CBA_fnc_ownerEvent;
													};

													case ((count _arguments) == 2): {
														private _binaryArguments = [missionNamespace, "KH_var_binaryArguments", _arguments, false] call KH_fnc_atomicVariable;
														[_id, [[] call (compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString ""))], _owner] call CBA_fnc_ownerEvent;
													};
												};
											}
											else {
												[_id, [_arguments call _parsedFunction], _owner] call CBA_fnc_ownerEvent;
											};
										}
										else {
											[_id, [_arguments call _function], _owner] call CBA_fnc_ownerEvent;
										};
									};

									switch true do {
										case (_callbackTarget isEqualType ""): {
											case (_callbackTarget == "SERVER"): {
												["KH_eve_executionServer", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction]] call CBA_fnc_serverEvent;
												true;
											};

											case (_callbackTarget == "ADMIN"): {
												if (KH_var_currentAdmin != 2) then {
													["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
													true;
												}
												else {
													false;
												};
											};

											case (((parseNumber (_callbackTarget select [0, 1])) != 0) && ([_callbackTarget] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
												if !(":" in _callbackTarget) then {
													if ((count _callbackTarget) == 17) then {
														private _player = KH_var_allPlayerUidMachines get _callbackTarget;

														if !(isNil "_player") then {
															["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _player] call CBA_fnc_ownerEvent;
															true;
														}
														else {
															false;
														};
													}
													else {
														private _player = KH_var_allPlayerIdMachines get _callbackTarget;

														if !(isNil "_player") then {
															["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _player] call CBA_fnc_ownerEvent;
															true;
														}
														else {
															private _client = KH_var_allHeadlessIdMachines get _callbackTarget;

															if !(isNil "_client") then {
																["KH_eve_executionHeadless", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _client] call CBA_fnc_ownerEvent;
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
														["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], objectFromNetId _callbackTarget] call CBA_fnc_targetEvent;
														true;
													}
													else {
														if !(isNull (groupFromNetId _callbackTarget)) then {
															[
																"KH_eve_executionServer", 
																[
																	[_callbackArguments, _callbackFunction, _callbackTarget, clientOwner],
																	{
																		params ["_callbackArguments", "_callbackFunction", "_callbackTarget", "_clientOwner"];
																		["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, _clientOwner], _callbackSendFunction], groupOwner (groupFromNetId _callbackTarget)] call CBA_fnc_ownerEvent;
																	}
																]
															] call CBA_fnc_serverEvent;

															true;
														}
														else {
															false;
														};											
													};
												};
											};

											default {
												private _unitPresent = false;

												{
													if ((name _x) == _callbackTarget) then {
														["KH_eve_executionPlayer", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _x] call CBA_fnc_targetEvent;
														_unitPresent = true;
														break;
													};
												} forEach KH_var_allPlayerUnits;
												
												_unitPresent;
											};
										};

										case (_callbackTarget isEqualType objNull): {
											if !(isNull _callbackTarget) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _callbackTarget] call CBA_fnc_targetEvent;
												true;
											}
											else {
												false;
											};
										};

										case (_callbackTarget isEqualType grpNull): {
											if !(isNull _callbackTarget) then {
												[
													"KH_eve_executionServer", 
													[
														[_callbackArguments, _callbackFunction, _callbackTarget, _callbackSendFunction, _id, clientOwner],
														{
															params ["_callbackArguments", "_callbackFunction", "_callbackTarget", "_callbackSendFunction", "_id", "_clientOwner"];
															["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, _clientOwner], _callbackSendFunction], groupOwner _callbackTarget] call CBA_fnc_ownerEvent;
														}
													]
												] call CBA_fnc_serverEvent;
												
												true;
											}
											else {
												false;
											};
										};

										case (_callbackTarget isEqualType teamMemberNull): {
											if !(isNull _callbackTarget) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], agent _callbackTarget] call CBA_fnc_targetEvent;
												true;
											}
											else {
												false;
											};
										};

										case (_callbackTarget isEqualType 0): {
											if (_callbackTarget in KH_var_allMachines) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _callbackTarget] call CBA_fnc_ownerEvent;
												true;
											}
											else {
												false;
											};
										};
									};
								};

								case (_type == "PERSISTENT"): {
									private _targetObject = _target select 1;

									if !(isNull _targetObject) then {
										private _persistencyFunction = {
											params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_functionProcessor", "_id"];

											[
												["STANDARD", _targetObject, false],
												"Local",
												[_arguments, _function, _sendoffArguments, _sendoffFunction, _functionProcessor, _id],
												{
													params ["_targetObject", "_isLocal"];
													_args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_functionProcessor", "_id"];
													private _idState = _targetObject getVariable [_id, "ACTIVE"];

													if !(_idState == "INACTIVE") then {
														switch true do {
															case (_idState == "ACTIVE"): {
																if _isLocal then {
																	[_arguments, _function] call _functionProcessor;
																}
																else {
																	[_sendoffArguments, _sendoffFunction] call _functionProcessor;
																};
															};

															case (_idState == "TERMINATE"): {
																[_eventName, _localId] call CBA_fnc_removeEventHandler;
															};
														};
													};
												}
											] call KH_fnc_addEventHandler;
										};

										private _sendoffArguments = _target param [2, []];
										private _sendoffFunction = _target param [3, {}];
										private _jip = _target param [4, true];
										private _id = "";

										if (_override != "") then {
											_id = _override;
										}
										else {
											_id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
										};

										["KH_eve_executionGlobal", [_arguments, _function], _targetObject] call CBA_fnc_targetEvent;

										{
											[
												"KH_eve_executionGlobal",
												[
													[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _functionProcessor, _id], 
													_persistencyFunction
												],
												_x
											] call CBA_fnc_ownerEvent;
										} forEach KH_var_allMachines;

										if _jip then {
											[
												"KH_eve_executionServer", 
												[
													[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, _functionProcessor, _id], 
													{
														params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_functionProcessor", "_id"];

														{
															[
																"CBA",
																_x,
																[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, _functionProcessor, _id],
																{
																	_args params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_functionProcessor", "_id"];
																	params ["_joiningMachine"];

																	[
																		"KH_eve_executionGlobal",
																		[
																			[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _functionProcessor, _id], 
																			_persistencyFunction
																		],
																		_joiningMachine
																	] call CBA_fnc_ownerEvent;
																}
															] call KH_fnc_addEventHandler;
														} forEach ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
													}
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

						case (_target isEqualType true): {
							if _target then {
								if hasInterface then {
									["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
								}
								else {
									if !isServer then {
										["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_globalEvent;
									}
									else {
										[_arguments, _function] call _functionProcessor;
									};
								};
							}
							else {
								if !hasInterface then {
									if !isServer then {
										["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
										["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
									}
									else {
										["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
										["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_globalEvent;
									};
								}
								else {
									["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
									["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_globalEvent;
								};
							};
							
							true;
						};

						case (_target isEqualType createHashMap): {
							{
								[_arguments, _function, _y select 1, _environment] call KH_fnc_execute;
								[_y select 0, _x, _y select 1, _y select 2] call KH_fnc_execute;
							} forEach _target;
							
							true;
						};

						default {
							false;
						};
					};
				};

				switch true do {
					case (_environment isEqualType ""): {
						switch true do {
							case (_environment == "THIS_FRAME"): {
								private _override = "";

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _override] call _subfunction;
								}
								else {
									private _result = true;

									if (_target isEqualType []) then {
										private _type = _target select 0;

										switch true do {
											case (_type == "JIP"): {
												_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
												_result = ["JIP_HANDLER", _override];
											};

											case (_type == "PERSISTENT"): {
												_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
												_result = ["PERSISTENT_HANDLER", _override, _target select 1];
											};
										};
									};

									KH_var_postInitExecutions pushBack [[_arguments, _function, _target, _override], _subfunction];
									_result;
								};
							};

							case (_environment == "NEXT_FRAME"): {
								private _result = true;
								private _override = "";
								
								if (_target isEqualType []) then {
									private _type = _target select 0;

									switch true do {
										case (_type == "JIP"): {
											_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
											_result = ["JIP_HANDLER", _override];
										};

										case (_type == "PERSISTENT"): {
											_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
											_result = ["PERSISTENT_HANDLER", _override, _target select 1];
										};
									};
								};

								private _callerFunction = {
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
									[_arguments, _function, _target, _subfunction, _override] call _callerFunction;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _subfunction, _override],
										_callerFunction
									];
								};

								_result;
							};

							default {
								false;
							};
						};
					};

					case (_environment isEqualType []): {
						private _result = true;
						private _override = "";
						
						if (_target isEqualType []) then {
							private _type = _target select 0;

							switch true do {
								case (_type == "JIP"): {
									_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
									_result = ["JIP_HANDLER", _override];
								};

								case (_type == "PERSISTENT"): {
									_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
									_result = ["PERSISTENT_HANDLER", _override, _target select 1];
								};
							};
						};

						private _type = _environment select 0;
						private _id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];

						switch true do {
							case (_type == "AFTER_N_FRAMES"): {
								private _callerFunction = {
									params ["_arguments", "_function", "_target", "_frames", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_override", "_id"];

									[
										{
											params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_override", "_id"];
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState != "TERMINATE") then {
												switch true do {
													case (_idState == "ACTIVE"): {
														if (_conditionFunction isEqualType "") then {
															_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
														};
														
														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															[_backupArguments, _backupFunction, _target, _override] call _subfunction;
														};
													};

													case (_idState == "INACTIVE"): {
														[_backupArguments, _backupFunction, _target, _override] call _subfunction;
													};
												};
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id],
										_frames
									] call CBA_fnc_execAfterNFrames;
								};

								private _frames = _environment select 1;
								private _conditionArguments = _environment param [2, []];
								private _conditionFunction = _environment param [3, {true;}];
								private _backupArguments = _environment param [4, []];
								private _backupFunction = _environment param [5, {}];

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _frames, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id] call _callerFunction;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _frames, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id],
										_callerFunction
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case (_type == "WAIT"): {
								private _callerFunction = {
									params ["_arguments", "_function", "_target", "_time", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_override", "_id"];

									[
										{
											params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_override", "_id"];
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState != "TERMINATE") then {
												switch true do {
													case (_idState == "ACTIVE"): {
														if (_conditionFunction isEqualType "") then {
															_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
														};

														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															[_backupArguments, _backupFunction, _target, _override] call _subfunction;
														};
													};

													case (_idState == "INACTIVE"): {
														[_backupArguments, _backupFunction, _target, _override] call _subfunction;
													};
												};
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id],
										_time
									] call CBA_fnc_waitAndExecute;
								};

								private _time = _environment select 1;
								private _conditionArguments = _environment param [2, []];
								private _conditionFunction = _environment param [3, {true;}];
								private _backupArguments = _environment param [4, []];
								private _backupFunction = _environment param [5, {}];

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _time, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id] call _callerFunction;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _time, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _override, _id],
										_callerFunction
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case (_type == "WAIT_UNTIL"): {
								private _callerFunction = {
									params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_override", "_id"];

									[
										{
											private _id = _this select 9;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											switch true do {
												case (_idState == "ACTIVE"): {
													private _conditionArguments = _this select 3;
													private _conditionFunction = _this select 4;

													if (_conditionFunction isEqualType "") then {
														_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
													};

													_conditionArguments call _conditionFunction;
												};

												case (_idState == "INACTIVE"): {
													false;
												};

												case (_idState == "TERMINATE"): {
													true;
												};
											};
										}, 
										{
											private _id = _this select 9;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState == "ACTIVE") then {
												private _arguments = _this select 0;
												private _function = _this select 1;
												private _target = _this select 2;
												private _subfunction = _this select 7;
												private _override = _this select 8;
												[_arguments, _function, _target, _override] call _subfunction;
											};
										}, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutArguments, _timeoutFunction, _subfunction, _override, _id],
										_timeout,
										{
											private _id = _this select 9;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];

											if (_idState == "ACTIVE") then {
												private _timeoutArguments = _this select 5;
												private _timeoutFunction = _this select 6;
												private _target = _this select 3;
												private _subfunction = _this select 7;
												private _override = _this select 8;
												[_timeoutArguments, _timeoutFunction, _target, _override] call _subfunction;
											};
										}
									] call CBA_fnc_waitUntilAndExecute;
								};

								private _conditionArguments = _environment select 1;
								private _conditionFunction = _environment select 2;
								private _timeout = _environment param [3, -1];
								private _timeoutArguments = _environment param [4, []];
								private _timeoutFunction = _environment param [5, {}];

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _subfunction, _override, _id] call _callerFunction;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _subfunction, _override, _id],
										_callerFunction
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
							};

							case (_type == "INTERVAL"): {
								private _callerFunction = {
									params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutOnConditionFailure", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_override", "_interval", "_id"];

									private _handler = [
										{
											private _id = _args select 10;
											private _idState = missionNamespace getVariable [_id, "ACTIVE"];
											
											if !(_idState == "INACTIVE") then {
												switch true do {
													case (_idState == "ACTIVE"): {
														_args params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeoutOnConditionFailure", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_override"];
														
														if (_conditionFunction isEqualType "") then {
															_conditionFunction = missionNamespace getVariable [_conditionFunction, {}];
														};
														
														if (_conditionArguments call _conditionFunction) then {
															[_arguments, _function, _target, _override] call _subfunction;
														}
														else {
															if _timeoutOnConditionFailure then {
																[_timeoutArguments, _timeoutFunction, _target, _override] call _subfunction;
																missionNamespace setVariable [_id, "TERMINATE"];
																[_handle] call CBA_fnc_removePerFrameHandler;
															};
														};
													};

													case (_idState == "TERMINATE"): {
														[_handle] call CBA_fnc_removePerFrameHandler;
													};		
												};
											};
										}, 
										_interval, 
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _subfunction, _override, _id]
									] call CBA_fnc_addPerFrameHandler;

									if (_timeout != 0) then {
										[
											{
												params ["_target", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_override", "_handler", "_id"];
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];
												
												if !(_idState == "TERMINATE") then {
													[_handler] call CBA_fnc_removePerFrameHandler;
													[_timeoutArguments, _timeoutFunction, _target, _override] call _subfunction;
												};
											}, 
											[_target, _timeoutArguments, _timeoutFunction, _subfunction, _override, _handler, _id], 
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

								if KH_var_missionLoaded then {
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _subfunction, _override, _interval, _id] call _callerFunction;
								}
								else {
									KH_var_postInitExecutions pushBack [
										[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutOnConditionFailure, _timeoutArguments, _timeoutFunction, _subfunction, _override, _interval, _id],
										_callerFunction
									];
								};

								[["PRIVATE_HANDLER", _id, clientOwner], _result];
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