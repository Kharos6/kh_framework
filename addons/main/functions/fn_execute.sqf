private "_return";

isNil {
	_return = (
		[_x] apply {
			params ["_arguments", "_function", ["_target", "GLOBAL"], ["_environment", "THIS_FRAME"]];

			private _subfunction = {
				params ["_arguments", "_function", "_target", ["_override", ""]];
				
				switch true do {
					case (_target isEqualType objNull): {
						if !(isNull _target) then {
							if (local _target) then {
								if (_function isEqualType "") then {
									_arguments call (missionNamespace getVariable [_function, {}]);
								}
								else {
									_arguments call _function;
								};

								true;
							}
							else {
								["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_targetEvent;
								true;
							};
						}
						else {
							false;
						};
					};

					case (_target isEqualType grpNull): {
						if !(isNull _target) then {
							{
								["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
							} forEach ([[_target], true] call KH_fnc_getClients);

							true;
						}
						else {
							false;
						};
					};
					
					case (_target isEqualType 0): {
						if (_target == clientOwner) then {
							if (_function isEqualType "") then {
								_arguments call (missionNamespace getVariable [_function, {}]);
							}
							else {
								_arguments call _function;
							};

							true;
						}
						else {
							if (_target in KH_var_allMachines) then {
								["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_ownerEvent;
								true;
							}
							else {
								false;
							};
						};
					};

					case (_target isEqualType sideUnknown): {
						["KH_eve_executionGlobal", [_arguments, _function], [[_target], true] call KH_fnc_getClients] call CBA_fnc_targetEvent;
						true;
					};

					case (_target isEqualType ""): {
						switch true do {
							case (_target == "SERVER"): {
								if (clientOwner == 2) then {
									if (_function isEqualType "") then {
										_arguments call (missionNamespace getVariable [_function, {}]);
									}
									else {
										_arguments call _function;
									};

									true;
								}
								else {
									["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
									true;
								};
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

									if !(isNull _curatorUnit) then {
										["KH_eve_executionGlobal", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
									};
								} forEach allCurators;

								true;
							};

							case (_target == "ADMIN"): {
								if (KH_var_currentAdmin != 2) then {
									if (KH_var_currentAdmin == clientOwner) then {
										if (_function isEqualType "") then {
											_arguments call (missionNamespace getVariable [_function, {}]);
										}
										else {
											_arguments call _function;
										};
									}
									else {
										["KH_eve_executionGlobal", [_arguments, _function], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
									};

									true;
								}
								else {
									false;
								};
							};

							case (((parseNumber (_target select [0, 1])) != 0) && ([_target] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
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
							};

							case (_target == "LOCAL"): {
								if (_function isEqualType "") then {
									_arguments call (missionNamespace getVariable [_function, {}]);
								}
								else {
									_arguments call _function;
								};
							};
							
							default {
								private _unitPresent = false;

								{
									if ((name _x) == _target) then {
										["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
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
					
						if (_type isEqualType "") then {
							switch true do {
								case (_type == "TARGETS"): {
									private _targets = _target select 1;
									private _failures = [];

									{
										switch true do {
											case (_x isEqualType objNull): {
												if !(isNull _x) then {
													["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
												}
												else {
													_failures pushBack _x;
												};
											};

											case (_x isEqualType grpNull): {
												private _group = _x;
												
												if !(isNull _group) then {
													{
														["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
													} forEach ([[_group], true] call KH_fnc_getClients);
												}
												else {
													_failures pushBack _x;
												};
											};
											
											case (_x isEqualType 0): {
												if (_x in KH_var_allMachines) then { 
													["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
												}
												else {
													_failures pushBack _x;
												};
											};

											case (_x isEqualType sideUnknown): {
												["KH_eve_executionGlobal", [_arguments, _function], [[_x], true] call KH_fnc_getClients] call CBA_fnc_targetEvent;
											};
											
											case (_x isEqualType ""): {
												switch true do {
													case (_x == "SERVER"): {
														["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
													};

													case (_x == "PLAYERS"): {
														["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
													};

													case (_x == "PLAYER_UNITS"): {
														{
															["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
														} forEach KH_var_allPlayerUnits;
													};

													case (_x == "HEADLESS"): {
														["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_globalEvent;
													};

													case (_x == "CURATORS"): {
														{
															private _curatorUnit = getAssignedCuratorUnit _x;

															if !(isNull _curatorUnit) then {
																["KH_eve_executionGlobal", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
															};
														} forEach allCurators;
													};

													case (_x == "ADMIN"): {
														if (KH_var_currentAdmin != 2) then {
															["KH_eve_executionGlobal", [_arguments, _function], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
														}
														else {
															_failures pushBack _x;
														};
													};

													case (((parseNumber (_x select [0, 1])) != 0) && ([_x] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
														if ((count _x) == 17) then {
															private _player = KH_var_allPlayerUidMachines get _x;

															if !(isNil "_player") then {
																["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
															}
															else {
																_failures pushBack _x;
															};
														}
														else {
															private _player = KH_var_allPlayerIdMachines get _x;

															if !(isNil "_player") then {
																["KH_eve_executionPlayer", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
															}
															else {
																private _client = KH_var_allHeadlessIdMachines get _x;

																if !(isNil "_client") then {
																	["KH_eve_executionHeadless", [_arguments, _function], _client] call CBA_fnc_ownerEvent;
																}
																else {
																	_failures pushBack _x;
																};
															};
														};
													};

													case (_x == "LOCAL"): {
														if (_function isEqualType "") then {
															_arguments call (missionNamespace getVariable [_function, {}]);	
														}
														else {
															_arguments call _function;
														};
													};

													default {
														private _unitPresent = false;
														private _current = _x;

														{
															if ((name _x) == _current) then {
																["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																_unitPresent = true;
																break;
															};
														} forEach KH_var_allPlayerUnits;
														
														if !_unitPresent then {
															_failures pushBack _x;
														};
													};
												};
											};

											case (_x isEqualType []): {
												private _currentTarget = _x;
												private _targetType = _currentTarget select 0;

												switch true do {
													case (_targetType == "GROUP_OWNER"): {
														private _group = _currentTarget select 1;

														if (_group isEqualType grpNull) then {
															if !(isNull _group) then {
																[
																	"KH_eve_executionGlobal", 
																	[
																		[_arguments, _function, _group],
																		{
																			params ["_arguments", "_function", "_group"];
																			["KH_eve_executionGlobal", [_arguments, _function], groupOwner _group] call CBA_fnc_ownerEvent;
																		}
																	]
																] call CBA_fnc_serverEvent;
															}
															else {
																failures pushBack _currentTarget;
															};
														}
														else {
															failures pushBack _currentTarget;
														};
													};

													case (_targetType == "PLAYER_ROLE"): {
														private _role = _currentTarget select 1;
														private _expectedRole = _currentTarget select 2;
														
														{
															if ((roleDescription _x) == _expectedRole) then {
																["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
															};
														} forEach KH_var_allPlayerUnits;
													};

													case (_targetType == "AREA_PLAYERS"): {
														private _area = _currentTarget select 1;
														private _invert = _currentTarget param [2, false];
														
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
													};

													default {
														failures pushBack _currentTarget;
													};
												};
											};

											default {
												_failures pushBack _x;
											};
										};
									} forEach _targets;

									_failures;
								};

								case (_type == "REMOTE"): {
									private _exclusiveType = _target select 1;

									switch true do {
										case (_exclusiveType == "GLOBAL"): {
											["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_remoteEvent;
											true;
										};

										case (_exclusiveType == "PLAYERS"): {
											["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_remoteEvent;
											true;
										};

										case (_exclusiveType == "PLAYER_UNITS"): {
											{
												["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
											} forEach (KH_var_allPlayerUnits - [player]);

											true;
										};

										case (_target == "CURATORS"): {
											{
												private _curatorUnit = getAssignedCuratorUnit _x;

												if !(isNull _curatorUnit) then {
													["KH_eve_executionGlobal", [_arguments, _function], _curatorUnit] call CBA_fnc_targetEvent;
												};
											} forEach (allCurators - ([getAssignedCuratorLogic player]));

											true;
										};

										case (_exclusiveType == "HEADLESS"): {
											["KH_eve_executionHeadless", [_arguments, _function]] call CBA_fnc_remoteEvent;
											true;
										};

										default {
											false;
										};
									};
								};

								case (_type == "GROUP_OWNER"): {
									private _group = _target select 1;

									if (_group isEqualType grpNull) then {
										if !(isNull _group) then {
											[
												"KH_eve_executionGlobal", 
												[
													[_arguments, _function, _group],
													{
														params ["_arguments", "_function", "_group"];
														["KH_eve_executionGlobal", [_arguments, _function], groupOwner _group] call CBA_fnc_ownerEvent;
													}
												]
											] call CBA_fnc_serverEvent;

											true;
										}
										else {
											false;
										};
									}
									else {
										false;
									};
								};

								case (_type == "PLAYER_ROLE"): {
									private _role = _target select 1;
									private _expectedRole = _target select 2;
									
									{
										if ((roleDescription _x) == _expectedRole) then {
											["KH_eve_executionPlayer", [_arguments, _function], _x] call CBA_fnc_targetEvent;
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

								case (_type == "JIP"): {
									private _exclusiveType = _target param [1, "GLOBAL"];
									private _dependency = _target param [2, true];
									private _unitRequired = _target param [3, true];
									private _remote = _target param [4, false];
									private _id = "";

									if (_override != "") then {
										_id = _override;
									}
									else {
										_id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
									};

									[
										"KH_eve_executionServer", 
										[
											[_arguments, _function, _exclusiveType, _dependency, _unitRequired, _remote, _id, clientOwner], 
											{
												params ["_arguments", "_function", "_exclusiveType", "_dependency", "_unitRequired", "_remote", "_id", "_originalOwner"];

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
																	["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																} forEach KH_var_allPlayerUnits;
																
																if isDedicated then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach ([2] + KH_var_allHeadlessMachines);
																}
																else {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
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
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_targetEvent;
																	};
																} forEach KH_var_allPlayerUnits;

																if isDedicated then {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
																	} forEach (([2] + KH_var_allHeadlessMachines) - [_originalOwner]);
																}
																else {
																	{
																		["KH_eve_executionGlobal", [_arguments, _function], _x] call CBA_fnc_ownerEvent;
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
												
												{
													[
														"CBA",
														_x,
														[_arguments, _function, _dependency, _id, _x],
														{
															_args params ["_arguments", "_function", "_dependency", "_id", "_joinType"];
															private _joiningMachine = 999999;

															if (_joinType == "KH_eve_playerLoaded") then {
																_joiningMachine = _this select 1;
															}
															else {
																_joiningMachine = _this select 0;
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

																case (_dependency isEqualType objNull): {
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
																						_condition = false;
																						break;
																					};
																				}
																				else {
																					if !(missionNamespace getVariable [_x, false]) then {
																						_condition = false;
																						break;
																					};
																				};
																			};

																			case (_x isEqualType objNull): {
																				if (isNull _x) then {
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
											[_id, [_arguments call (missionNamespace getVariable [_function, {}])], _owner] call CBA_fnc_ownerEvent;
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
													["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
													true;
												}
												else {
													false;
												};
											};

											case (((parseNumber (_callbackTarget select [0, 1])) != 0) && ([_callbackTarget] call {params ["_target"]; private _condition = true; {if ((name _x) == _target) then {_condition = false; break;};} forEach KH_var_allPlayerUnits; _condition;})): {
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
											};

											default {
												private _unitPresent = false;

												{
													if ((name _x) == _callbackTarget) then {
														["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _x] call CBA_fnc_targetEvent;
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

										case (_callbackTarget isEqualType 0): {
											if (_callbackTarget in KH_var_allMachines) then {
												["KH_eve_executionGlobal", [[_callbackArguments, _callbackFunction, _id, clientOwner], _callbackSendFunction], _callbackTarget] call CBA_fnc_ownerEvent;
												true;
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

								case (_type == "PERSISTENT"): {
									private _targetObject = _target select 1;

									if (_targetObject isEqualType objNull) then {
										if !(isNull _targetObject) then {
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
														[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _id], 
														{
															params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_id"];

															[
																["STANDARD", _targetObject, false],
																"Local",
																[_arguments, _function, _sendoffArguments, _sendoffFunction, _id],
																{
																	params ["_targetObject", "_isLocal"];
																	_args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_id"];
																	private _idState = _targetObject getVariable [_id, "ACTIVE"];

																	if !(_idState == "INACTIVE") then {
																		switch true do {
																			case (_idState == "ACTIVE"): {
																				if _isLocal then {
																					if (_function isEqualType "") then {
																						_arguments call (missionNamespace getVariable [_function, {}]);
																					}
																					else {
																						_arguments call _function;
																					};
																				}
																				else {
																					if (_sendoffFunction isEqualType "") then {
																						_sendoffArguments call (missionNamespace getVariable [_sendoffFunction, {}]);
																					}
																					else {
																						_sendoffArguments call _sendoffFunction;
																					};
																				};
																			};

																			case (_idState == "TERMINATE"): {
																				[_eventName, _localId] call CBA_fnc_removeEventHandler;
																			};
																		};
																	};
																}
															] call KH_fnc_addEventHandler;
														}
													],
													_x
												] call CBA_fnc_ownerEvent;
											} forEach KH_var_allMachines;

											if _jip then {
												[
													"KH_eve_executionServer", 
													[
														[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _id], 
														{
															params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_id"];

															{
																[
																	"CBA",
																	_x,
																	[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _id],
																	{
																		_args params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_id"];
																		params ["_joiningMachine"];

																		[
																			"KH_eve_executionGlobal",
																			[
																				[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _id], 
																				{
																					params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_id"];

																					[
																						["STANDARD", _targetObject, false],
																						"Local",
																						[_arguments, _function, _sendoffArguments, _sendoffFunction, _id],
																						{
																							params ["_targetObject", "_isLocal"];
																							_args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_id"];
																							private _idState = _targetObject getVariable [_id, "ACTIVE"];
																							
																							if !(_idState == "INACTIVE") then {
																								switch true do {
																									case (_idState == "ACTIVE"): {
																										if _isLocal then {
																											if (_function isEqualType "") then {
																												_arguments call (missionNamespace getVariable [_function, {}]);
																											}
																											else {
																												_arguments call _function;
																											};
																										}
																										else {
																											if (_sendoffFunction isEqualType "") then {
																												_sendoffArguments call (missionNamespace getVariable [_sendoffFunction, {}]);
																											}
																											else {
																												_sendoffArguments call _sendoffFunction;
																											};
																										};
																									};

																									case (_idState == "TERMINATE"): {
																										[_eventName, _localId] call CBA_fnc_removeEventHandler;
																									};
																								};
																							};
																						}
																					] call KH_fnc_addEventHandler;
																				}
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
									}
									else {
										false;
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
							if KH_var_missionLoaded then {
								[_arguments, _function, _target] call _subfunction;
							}
							else {
								private _return = true;
								private _valid = true;
								private _override = "";

								if (_target isEqualType []) then {
									private _type = _target select 0;

									if (_type isEqualType "") then {
										switch true do {
											case (_type == "TARGETS"): {
												_return = [];
											};

											case (_type == "JIP"): {
												_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
												_return = ["JIP_HANDLER", _override];
											};

											case (_type == "PERSISTENT"): {
												_override = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
												_return = ["PERSISTENT_HANDLER", _override, _target select 1];
											};
										};
									}
									else {
										_valid = false;
									};
								};

								if _valid then {
									KH_var_postInitExecutions pushBack [[_arguments, _function, _target, _override], _subfunction];
									_return;
								}
								else {
									false;
								};
							};
						};

						case (_environment == "NEXT_FRAME"): {
							if KH_var_missionLoaded then {
								[
									{
										params ["_arguments", "_function", "_target", "_subfunction"];
										[_arguments, _function, _target] call _subfunction;
									}, 
									[_arguments, _function, _target, _subfunction]
								] call CBA_fnc_execNextFrame;
							}
							else {
								KH_var_postInitExecutions pushBack [
									[_arguments, _function, _target, _subfunction],
									{
										params ["_arguments", "_function", "_target", "_subfunction"];

										[
											{
												params ["_arguments", "_function", "_target", "_subfunction"];
												[_arguments, _function, _target] call _subfunction;
											}, 
											[_arguments, _function, _target, _subfunction]
										] call CBA_fnc_execNextFrame;
									}
								];
							};

							true;
						};

						default {
							false;
						};
					};
				};

				case (_environment isEqualType []): {
					private _type = _environment select 0;
					private _id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
					missionNamespace setVariable [_id, "ACTIVE"];

					switch true do {
						case (_type == "AFTER_N_FRAMES"): {
							private _frames = _environment select 1;
							private _conditionArguments = _environment param [2, []];
							private _conditionFunction = _environment param [3, {true;}];
							private _backupArguments = _environment param [4, []];
							private _backupFunction = _environment param [5, {}];

							if KH_var_missionLoaded then {
								[
									{
										params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState != "TERMINATE") then {
											switch true do {
												case (_idState == "ACTIVE"): {
													if (_conditionArguments call _conditionFunction) then {
														[_arguments, _function, _target] call _subfunction;
													}
													else {
														[_backupArguments, _backupFunction, _target] call _subfunction;
													};
												};

												case (_idState == "INACTIVE"): {
													[_backupArguments, _backupFunction, _target] call _subfunction;
												};
											};
										};
									}, 
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
									_frames
								] call CBA_fnc_execAfterNFrames;
							}
							else {
								KH_var_postInitExecutions pushBack [
									[_arguments, _function, _target, _frames, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
									{
										params ["_arguments", "_function", "_target", "_frames", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];

										[
											{
												params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState != "TERMINATE") then {
													switch true do {
														case (_idState == "ACTIVE"): {
															if (_conditionArguments call _conditionFunction) then {
																[_arguments, _function, _target] call _subfunction;
															}
															else {
																[_backupArguments, _backupFunction, _target] call _subfunction;
															};
														};

														case (_idState == "INACTIVE"): {
															[_backupArguments, _backupFunction, _target] call _subfunction;
														};
													};
												};
											}, 
											[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
											_frames
										] call CBA_fnc_execAfterNFrames;
									}
								];
							};

							["PRIVATE_HANDLER", _id, clientOwner];
						};

						case (_type == "WAIT"): {
							private _time = _environment select 1;
							private _conditionArguments = _environment param [2, []];
							private _conditionFunction = _environment param [3, {true;}];
							private _backupArguments = _environment param [4, []];
							private _backupFunction = _environment param [5, {}];

							if KH_var_missionLoaded then {
								[
									{
										params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState != "TERMINATE") then {
											switch true do {
												case (_idState == "ACTIVE"): {
													if (_conditionArguments call _conditionFunction) then {
														[_arguments, _function, _target] call _subfunction;
													}
													else {
														[_backupArguments, _backupFunction, _target] call _subfunction;
													};
												};

												case (_idState == "INACTIVE"): {
													[_backupArguments, _backupFunction, _target] call _subfunction;
												};
											};
										};
									}, 
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
									_time
								] call CBA_fnc_waitAndExecute;
							}
							else {
								KH_var_postInitExecutions pushBack [
									[_arguments, _function, _target, _time, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
									{
										params ["_arguments", "_function", "_target", "_time", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
										
										[
											{
												params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState != "TERMINATE") then {
													switch true do {
														case (_idState == "ACTIVE"): {
															if (_conditionArguments call _conditionFunction) then {
																[_arguments, _function, _target] call _subfunction;
															}
															else {
																[_backupArguments, _backupFunction, _target] call _subfunction;
															};
														};

														case (_idState == "INACTIVE"): {
															[_backupArguments, _backupFunction, _target] call _subfunction;
														};
													};
												};
											}, 
											[_arguments, _function, _target, _conditionArguments, _conditionFunction, _backupArguments, _backupFunction, _subfunction, _id],
											_time
										] call CBA_fnc_waitAndExecute;
									}
								];
							};

							["PRIVATE_HANDLER", _id, clientOwner];
						};

						case (_type == "WAIT_UNTIL"): {
							private _conditionArguments = _environment select 1;
							private _conditionFunction = _environment select 2;
							private _timeout = _environment param [3, -1];
							private _timeoutArguments = _environment param [4, []];
							private _timeoutFunction = _environment param [5, {}];

							if KH_var_missionLoaded then {
								[
									{
										private _id = _this select 8;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										switch true do {
											case (_idState == "ACTIVE"): {
												private _conditionArguments = _this select 3;
												private _conditionFunction = _this select 4;
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
										private _id = _this select 8;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState == "ACTIVE") then {
											private _arguments = _this select 0;
											private _function = _this select 1;
											private _target = _this select 2;
											private _subfunction = _this select 7;
											[_arguments, _function, _target] call _subfunction;
										};
									}, 
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutArguments, _timeoutFunction, _subfunction, _id],
									_timeout,
									{
										private _id = _this select 8;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState == "ACTIVE") then {
											private _timeoutArguments = _this select 5;
											private _timeoutFunction = _this select 6;
											private _target = _this select 3;
											private _subfunction = _this select 7;
											[_timeoutArguments, _timeoutFunction, _target] call _subfunction;
										};
									}
								] call CBA_fnc_waitUntilAndExecute;
							}
							else {
								KH_var_postInitExecutions pushBack [
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _subfunction, _id],
									{
										params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_id"];

										[
											{
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												switch true do {
													case (_idState == "ACTIVE"): {
														private _conditionArguments = _this select 3;
														private _conditionFunction = _this select 4;
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
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState == "ACTIVE") then {
													private _arguments = _this select 0;
													private _function = _this select 1;
													private _target = _this select 2;
													private _subfunction = _this select 7;
													[_arguments, _function, _target] call _subfunction;
												};
											}, 
											[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeoutArguments, _timeoutFunction, _subfunction, _id],
											_timeout,
											{
												private _id = _this select 8;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];

												if (_idState == "ACTIVE") then {
													private _timeoutArguments = _this select 5;
													private _timeoutFunction = _this select 6;
													private _target = _this select 3;
													private _subfunction = _this select 7;
													[_timeoutArguments, _timeoutFunction, _target] call _subfunction;
												};
											}
										] call CBA_fnc_waitUntilAndExecute;
									}
								];
							};

							["PRIVATE_HANDLER", _id, clientOwner];
						};

						case (_type == "INTERVAL"): {
							private _interval = _environment select 1;
							private _conditonArguments = _environment param [2, []];
							private _conditionFunction  = _environment param [3, {true;}];
							private _timeout = _environment param [4, 0];
							private _timeoutArguments = _environment param [5, []];
							private _timeoutFunction = _environment param [6, {}];

							if KH_var_missionLoaded then {
								private _handler = [
									{
										private _id = _args select 6;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];
										
										if !(_idState == "INACTIVE") then {
											switch true do {
												case (_idState == "ACTIVE"): {
													_args params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_subfunction"];

													if (_conditionArguments call _conditionFunction) then {
														[_arguments, _function, _target] call _subfunction;
													};
												};

												case (_idState == "TERMINATE"): {
													[_handle] call CBA_fnc_removePerFrameHandler;
												};		
											};
										};
									}, 
									_interval, 
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _subfunction, _id]
								] call CBA_fnc_addPerFrameHandler;

								if (_timeout != 0) then {
									[
										{
											params ["_timeoutArguments", "_timeoutFunction", "_subfunction", "_handler"];
											[_handler] call CBA_fnc_removePerFrameHandler;
											[_timeoutArguments, _timeoutFunction, _target] call _subfunction;	
										}, 
										[_timeoutArguments, _timeoutFunction, _subfunction, _handler], 
										_timeout
									] call CBA_fnc_waitAndExecute;
								};
							}
							else {
								KH_var_postInitExecutions pushBack [
									[_arguments, _function, _target, _conditionArguments, _conditionFunction, _timeout, _timeoutArguments, _timeoutFunction, _subfunction, _interval, _id],
									{
										params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_timeout", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_interval", "_id"];
										
										private _handler = [
											{
												private _id = _args select 6;
												private _idState = missionNamespace getVariable [_id, "ACTIVE"];
												
												if !(_idState == "INACTIVE") then {
													switch true do {
														case (_idState == "ACTIVE"): {
															_args params ["_arguments", "_function", "_target", "_conditionArguments", "_conditionFunction", "_subfunction"];

															if (_conditionArguments call _conditionFunction) then {
																[_arguments, _function, _target] call _subfunction;
															};
														};

														case (_idState == "TERMINATE"): {
															[_handle] call CBA_fnc_removePerFrameHandler;
														};		
													};
												};
											}, 
											_interval, 
											[_arguments, _function, _target, _conditionArguments, _conditionFunction, _subfunction, _id]
										] call CBA_fnc_addPerFrameHandler;

										if (_timeout != 0) then {
											[
												{
													params ["_timeoutArguments", "_timeoutFunction", "_subfunction", "_handler"];
													[_handler] call CBA_fnc_removePerFrameHandler;
													[_timeoutArguments, _timeoutFunction, _target] call _subfunction;	
												}, 
												[_timeoutArguments, _timeoutFunction, _subfunction, _handler], 
												_timeout
											] call CBA_fnc_waitAndExecute;
										};
									}
								];
							};

							["PRIVATE_HANDLER", _id, clientOwner];
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
	) select 0;
};

if !(isNil "_return") then {
	_return;
}
else {
	false;
};