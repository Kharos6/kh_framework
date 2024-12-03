private "_return";

isNil {
	_return = (
		[_x] apply {
			_this call {
				params ["_arguments", "_function", ["_target", "LOCAL"], ["_environment", "THIS_FRAME"]];

				private _subfunction = {
					params ["_arguments", "_function", "_target"];
					
					switch true do {
						case (_target isEqualType objNull): {
							if !(isNull _target) then {
								["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_targetEvent;
								true;
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
							}
							else {
								false;
							};
						};
						
						case (_target isEqualType 0): {
							if (_target in KH_var_allMachines) then {
								["KH_eve_executionGlobal", [_arguments, _function], _target] call CBA_fnc_ownerEvent;
								true;
							}
							else {
								false;
							};
						};

						case (_target isEqualType sideUnknown): {
							["KH_eve_executionGlobal", [_arguments, _function], [[_target], true] call KH_fnc_getClients] call CBA_fnc_targetEvent;
							true;
						};

						case (_target isEqualType ""): {
							switch true do {
								case (_target == "SERVER"): {
									["KH_eve_executionServer", [_arguments, _function]] call CBA_fnc_serverEvent;
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
								};

								case (_target == "ADMIN"): {
									if (KH_var_currentAdmin != 2) then {
										["KH_eve_executionGlobal", [_arguments, _function], KH_var_currentAdmin] call CBA_fnc_ownerEvent;
										true;
									}
									else {
										false;
									};
								};

								case ((parseNumber (_target select [0, 1])) != 0): {
									private _player = KH_var_allPlayerMachineUids get _target;

									if !(isNil _player) then {
										["KH_eve_executionGlobal", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
										true;
									}
									else {
										false;
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
									} forEach ([["ALL"], true] call KH_fnc_getClients);
									
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

														case ((parseNumber (_x select [0, 1])) != 0): {
															private _player = KH_var_allPlayerMachineUids get _x;

															if !(isNil _player) then {
																["KH_eve_executionGlobal", [_arguments, _function], _player] call CBA_fnc_ownerEvent;
															}
															else {
																_failures pushBack _x;
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
															} forEach ([["ALL"], true] call KH_fnc_getClients);
															
															if !_unitPresent then {
																_failures pushBack _x;
															};
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
												["KH_eve_executionGlobal", [_arguments, _function], leader _group] call CBA_fnc_targetEvent;
											}
											else {
												false;
											};
										}
										else {
											false;
										};
									};

									case (_type == "JIP"): {
										private _exclusiveType = _target select 1;
										private _dependency = _target select 2;
										private _unitRequired = _target select 3;
										private _remote = _target select 4;
										private _id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
										
										private _jipFunction = {
											params ["_arguments", "_function", "_dependency", "_unitRequired", "_id"];

											[
												"KH_eve_executionGlobal", 
												[
													[_arguments, _function, _dependency, _unitRequired, _id], 
													{
														params ["_arguments", "_function", "_dependency", "_unitRequired", "_id"];
														missionNamespace setVariable [_id, "ACTIVE"];
														private _joinType = "";
														
														if _unitRequired then {
															_joinType = "KH_eve_playerLoaded";
														}
														else {
															_joinType = "KH_eve_playerPreloaded";
														};
														
														[
															_joinType, 
															{
																_thisArgs params ["_arguments", "_function", "_dependency", "_id", "_joinType"];
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
																			private _uid = _this select 3;

																			if (_dependency == _uid) then {
																				if !(_idState == "INACTIVE") then {
																					switch true do {
																						case (_idState == "ACTIVE"): {
																							["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																						};

																						case (_idState == "TERMINATE"): {
																							[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																						};		
																					};
																				};
																			};
																		}
																		else {
																			if (missionNamespace getVariable [_dependency, true]) then {
																				private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																				if !(_idState == "INACTIVE") then {
																					switch true do {
																						case (_idState == "ACTIVE"): {
																							["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																						};

																						case (_idState == "TERMINATE"): {
																							[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																						};		
																					};
																				};
																			}
																			else {
																				missionNamespace setVariable [_id, "INACTIVE"];
																			};
																		};
																	};

																	case (_dependency isEqualType objNull): {
																		if !(isNil "_dependency") then {
																			private _idState = missionNamespace getVariable [_id, "ACTIVE"];

																			if !(_idState == "INACTIVE") then {
																				switch true do {
																					case (_idState == "ACTIVE"): {
																						["KH_eve_executionGlobal", [_arguments, _function], _joiningMachine] call CBA_fnc_ownerEvent;
																					};

																					case (_idState == "TERMINATE"): {
																						[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		}
																		else {
																			missionNamespace setVariable [_id, "INACTIVE"];
																		};
																	};

																	case (_dependency isEqualType []): {
																		private _condition = true;
																		
																		{
																			switch true do {
																				case (_x isEqualType ""): {
																					if ((parseNumber (_dependency select [0, 1])) != 0) then {
																						private _uid = _this select 3;

																						if (_dependency != _uid) then {
																							_condition = false;
																							break;
																						};
																					}
																					else {
																						if !(missionNamespace getVariable [_dependency, true]) then {
																							_condition = false;
																							break;
																						}
																					};
																				};

																				case (_x isEqualType objNull): {
																					if (isNil "_dependency") then {
																						_condition = false;
																						break;
																					}
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
																						[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																					};		
																				};
																			};
																		}
																		else {
																			missionNamespace setVariable [_id, "INACTIVE"];
																		};
																	};
																};
															}, 
															[_arguments, _function, _dependency, _id, _joinType]
														] call CBA_fnc_addEventHandlerArgs;
													}
												]
											] call CBA_fnc_serverEvent;
										};

										if (CBA_missionTime > 0) then {
											if !KH_var_playersInitialized then {
												[
													{
														KH_var_playersInitialized;
													},
													{
														params ["_arguments", "_function", "_dependency", "_unitRequired", "_id"];
														[_arguments, _function, _dependency, _unitRequired, _id] call _jipFunction;
													},
													[_arguments, _function, _dependency, _unitRequired, _id]
												] call CBA_fnc_waitUntilAndExecute;
											}
											else {
												[_arguments, _function, _dependency, _unitRequired, _id] call _jipFunction;
											};
										}
										else {
											[_arguments, _function, _dependency, _unitRequired, _id] call _jipFunction;
										};
										
										switch true do {
											case (_exclusiveType == "GLOBAL"): {
												if !_remote then {
													["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_globalEvent;
												}
												else {
													["KH_eve_executionGlobal", [_arguments, _function]] call CBA_fnc_remoteEvent;
												};

												_id;
											};

											case (_exclusiveType == "PLAYERS"): {
												if !_remote then {
													["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_globalEvent;
												}
												else {
													["KH_eve_executionPlayer", [_arguments, _function]] call CBA_fnc_remoteEvent;
												};

												_id;
											};

											case (_exclusiveType == "ONLY_JIP"): {
												_id;
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
												private _targetIndex = _target select 2;
												private _sendoffArguments = _target select 3;
												private _sendoffFunction = _target select 4;
												private _jip = _target select 5;
												private _id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
												_targetObject setVariable [_id, "ACTIVE", true];
												["KH_eve_executionGlobal", [_arguments, _function], _targetObject] call CBA_fnc_targetEvent;

												[
													"KH_eve_executionGlobal",
													[
														[_arguments, _function, _targetObject, _targetIndex, _sendoffArguments, _sendoffFunction, _id], 
														{
															params ["_arguments", "_function", "_targetObject", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];

															[
																"KH_eve_localityChanged", 
																{
																	params ["_targetObject", "_isLocal"];
																	_thisArgs params ["_arguments", "_function", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];
																	private _idState = _targetObject getVariable [_id, "ACTIVE"];

																	if !(_idState == "INACTIVE") then {
																		switch true do {
																			case (_idState == "ACTIVE"): {
																				if _isLocal then {
																					_arguments insert [_targetIndex, [_targetObject]];

																					if (_function isEqualType "") then {
																						_arguments call (missionNamespace getVariable [_function, {}]);
																					}
																					else {
																						_arguments call _function;
																					};
																				}
																				else {
																					_sendoffArguments insert [_targetIndex, [_targetObject]];

																					if (_sendoffFunction isEqualType "") then {
																						_sendoffArguments call (missionNamespace getVariable [_sendoffFunction, {}]);
																					}
																					else {
																						_sendoffArguments call _function;
																					};
																				};
																			};

																			case (_idState == "TERMINATE"): {
																				[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																			};
																		};
																	};
																}, 
																[_arguments, _function, _targetIndex, _sendoffArguments, _sendoffFunction, _id]
															] call CBA_fnc_addEventHandlerArgs;

															_targetObject addEventHandler [
																"Local", 
																{
																	params ["_entity", "_isLocal"];
																	["KH_eve_localityChanged", [_entity, _isLocal]] call CBA_fnc_localEvent;
																}
															];
														}
													]
												] call CBA_fnc_globalEvent;

												if _jip then {
													[
														"KH_eve_executionGlobal", 
														[
															[_arguments, _function, _targetObject, _targetIndex, _sendoffArguments, _sendoffFunction, _id], 
															{
																params ["_arguments", "_function", "_targetObject", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];

																[
																	"KH_eve_playerPreLoaded", 
																	{
																		_thisArgs params ["_arguments", "_function", "_targetObject", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];
																		params ["_joiningMachine"];

																		[
																			"KH_eve_executionGlobal",
																			[
																				[_arguments, _function, _targetObject, _targetIndex, _sendoffArguments, _sendoffFunction, _id], 
																				{
																					params ["_arguments", "_function", "_targetObject", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];

																					[
																						"KH_eve_localityChanged", 
																						{
																							params ["_targetObject", "isLocal"];
																							_thisArgs params ["_arguments", "_function", "_targetIndex", "_sendoffArguments", "_sendoffFunction", "_id"];
																							private _idState = _targetObject getVariable [_id, "ACTIVE"];
																							
																							if !(_idState == "INACTIVE") then {
																								switch true do {
																									case (_idState == "ACTIVE"): {
																										if _isLocal then {
																											_arguments insert [_targetIndex, [_targetObject]];

																											if (_function isEqualType "") then {
																												_arguments call (missionNamespace getVariable [_function, {}]);
																											}
																											else {
																												_arguments call _function;
																											};
																										}
																										else {
																											_sendoffArguments insert [_targetIndex, [_targetObject]];

																											if (_sendoffFunction isEqualType "") then {
																												_sendoffArguments call (missionNamespace getVariable [_sendoffFunction, {}]);
																											}
																											else {
																												_sendoffArguments call _function;
																											};
																										};
																									};

																									case (_idState == "TERMINATE"): {
																										[_thisType, _thisId] call CBA_fnc_removeEventHandler;
																									};
																								};
																							};
																						}, 
																						[_arguments, _function, _targetIndex, _sendoffArguments, _sendoffFunction, _id]
																					] call CBA_fnc_addEventHandlerArgs;

																					_targetObject addEventHandler [
																						"Local", 
																						{
																							params ["_entity", "_isLocal"];
																							["KH_eve_localityChanged", [_entity, _isLocal]] call CBA_fnc_localEvent;
																						}
																					];
																				}
																			],
																			_joiningMachine
																		] call CBA_fnc_ownerEvent;
																	}, 
																	[_arguments, _function, _targetObject, _targetIndex, _sendoffArguments, _sendoffFunction, _id]
																] call CBA_fnc_addEventHandlerArgs;
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
								[_arguments, _function, _target] call _subfunction;
							};

							case (_environment == "NEXT_FRAME"): {
								[
									{
										params ["_arguments", "_function", "_target", "_subfunction"];
										[_arguments, _function, _target] call _subfunction;
									}, 
									[_arguments, _function, _target, _subfunction]
								] call CBA_fnc_execNextFrame;

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
							case (_type == "WAIT"): {
								private _time = _environment select 1;
								private _backupArguments = _environment select 2;
								private _backupFunction = _environment select 3;

								[
									{
										params ["_arguments", "_function", "_target", "_backupArguments", "_backupFunction", "_subfunction", "_id"];
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState != "TERMINATE") then {
											switch true do {
												case (_idState == "ACTIVE"): {
													[_arguments, _function, _target] call _subfunction;
												};

												case (_idState == "INACTIVE"): {
													[_backupArguments, _backupFunction, _target] call _subfunction;
												};
											};
										};
									}, 
									[_arguments, _function, _target, _backupArguments, _backupFunction, _subfunction, _id],
									_time
								] call CBA_fnc_waitAndExecute;

								["PRIVATE_HANDLER", _id, clientOwner];
							};

							case (_type == "WAIT_UNTIL"): {
								private _conditionCode = _environment select 1;
								private _timeout = _environment select 2;
								private _timeoutArguments = _environment select 3;
								private _timeoutFunction = _environment select 4;

								[
									{
										private _id = _this select 7;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										switch true do {
											case (_idState == "ACTIVE"): {
												private _conditionCode = _this select 3;
												[] call _conditionCode;
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
										private _id = _this select 7;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState == "ACTIVE") then {
											private _arguments = _this select 0;
											private _function = _this select 1;
											private _target = _this select 2;
											private _subfunction = _this select 6;
											[_arguments, _function, _target] call _subfunction;
										};
									}, 
									[_arguments, _function, _target, _conditionCode, _timeoutArguments, _timeoutFunction, _subfunction, _id],
									_timeout,
									{
										private _id = _this select 7;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];

										if (_idState == "ACTIVE") then {
											private _timeoutArguments = _this select 4;
											private _timeoutFunction = _this select 5;
											private _target = _this select 3;
											private _subfunction = _this select 6;
											[_timeoutArguments, _timeoutFunction, _target] call _subfunction;
										};
									}
								] call CBA_fnc_waitUntilAndExecute;

								["PRIVATE_HANDLER", _id, clientOwner];
							};

							case (_type == "INTERVAL"): {
								private _interval = _environment select 1;

								[
									{
										private _id = _args select 4;
										private _idState = missionNamespace getVariable [_id, "ACTIVE"];
										
										if !(_idState == "INACTIVE") then {
											switch true do {
												case (_idState == "ACTIVE"): {
													_args params ["_arguments", "_function", "_target", "_subfunction"];
													[_arguments, _function, _target] call _subfunction;	
												};

												case (_idState == "TERMINATE"): {
													[_handle] call CBA_fnc_removePerFrameHandler;
												};		
											};
										};
									}, 
									_interval, 
									[_arguments, _function, _target, _subfunction, _id]
								] call CBA_fnc_addPerFrameHandler;

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
		}
	) select 0;
};

if !(isNil "_return") then {
	_return;
}
else {
	false;
};