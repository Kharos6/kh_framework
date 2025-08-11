params [
	["_namespace", missionNamespace, ["", missionNamespace, objNull, grpNull, teamMemberNull, locationNull, taskNull, controlNull, displayNull]], 
	["_name", nil], 
	["_value", nil], 
	["_scope", "LOCAL", [true, 0, "", [], objNull, teamMemberNull, grpNull, sideUnknown]], 
	["_writeType", "OVERWRITE", [""]]
];

private _handler = [];

if (_namespace isEqualType []) exitWith {
	private _i = 0;

	for "_i" from 0 to ((count _namespace) - 1) do {
		_handler pushBack ([_namespace select _i, _name select _i, _value select _i, _scope select _i, _writeType select _i] call KH_fnc_setVariable);
	};

	_handler;
};

if (_scope isEqualType "") then {
	private "_parsedName";
	private _continue = true;
	private _khData = false;

	if (_namespace isEqualType "") then {
		_parsedName = switch (typeName _name) do {
			case "OBJECT": {
				if (isPlayer _name) then {
					["player_", getPlayerUID _name] joinString "";
				}
				else {
					if ((vehicleVarName _name) isEqualTo "") then {
						if ((roleDescription _name) isEqualTo "") then {
							"";
						}
						else {
							roleDescription _name;
						};
					}
					else {
						vehicleVarName _name;
					};
				};
			};

			case "TEAM_MEMBER": {
				private _entity = agent _name;

				if (isPlayer _entity) then {
					getPlayerUID _entity;
				}
				else {
					if ((vehicleVarName _entity) isEqualTo "") then {
						if ((roleDescription _entity) isEqualTo "") then {
							"";
						}
						else {
							roleDescription _entity;
						};
					}
					else {
						vehicleVarName _entity;
					};
				};
			};

			case "GROUP": {
				groupId _name;
			};

			case "LOCATION": {
				name _name;
			};
			
			case "STRING": {
				_name;
			};

			default {
				str _name;
			};
		};

		if (_parsedName isEqualTo "") then {
			_continue = false;
		}
		else {
			_khData = true;
		};
	}
	else {
		_parsedName = _name;
	};

	if !_continue exitWith {
		nil;
	};

	switch _scope do {
		case "LOCAL": {
			private "_parsedValue";

			private _oldValue = if _khData then {
				[_namespace, _parsedName, nil, []] call KH_fnc_getVariable;
			}
			else {
				_namespace getVariable _parsedName;
			};

			if !(isNil "_oldValue") then {
				_parsedValue = switch _writeType do {
					case "OVERWRITE": {
						_value;
					};

					case "ADD": {
						_oldValue + _value;
					};

					case "SUBTRACT": {
						_oldValue - _value;
					};

					case "MULTIPLY": {
						_oldValue * _value;
					};

					case "DIVIDE": {
						_oldValue / _value;
					};

					case "RAISE": {
						_oldValue ^ _value;
					};

					case "REMAINDER": {
						_oldValue % _value;
					};

					case "INTERSECT": {
						_oldValue arrayIntersect _value;
					};

					case "APPEND": {
						_oldValue append _value;
						_oldValue;
					};

					case "PUSHBACK": {
						_oldValue pushBack _value;
						_oldValue;
					};

					case "PUSHBACK_UNIQUE": {
						_oldValue pushBackUnique _value;
						_oldValue;
					};

					case "SET": {
						_oldValue set [_value select 0, _value select 1, false];
						_oldValue;
					};

					case "SET_UNIQUE": {
						_oldValue set [_value select 0, _value select 1, true];
						_oldValue;
					};

					case "INSERT": {
						_oldValue insert _value;
						_oldValue;
					};

					case "INSERT_UNIQUE": {
						_oldValue insert [_value select 0, _value select 1, true];
						_oldValue;
					};

					case "MERGE_OVERWRITE": {
						_oldValue merge [_value, true];
						_oldValue;
					};

					case "MERGE": {
						_oldValue merge [_value, false];
						_oldValue;
					};
				};

				if ((_parsedValue isEqualType createHashMap) || (_parsedValue isEqualType [])) then {
					_continue = false;
				};
			}
			else {
				_parsedValue = _value;
			};

			if _continue then {
				if !_khData then {
					_namespace setVariable [_parsedName, _parsedValue];
				}
				else {				
					[_namespace, _parsedName, _parsedValue, typeName _parsedValue, true] call KH_fnc_writeKhData;
				};
			};

			["KH_eve_variableChanged", [_namespace, _parsedName, _parsedValue, _oldValue, _writeType]] call CBA_fnc_localEvent;
		};

		case "PERSISTENT": {
			if _khData exitWith {};
			private "_parsedValue";
			private _oldValue = _namespace getVariable _parsedName;

			if !(isNil "_oldValue") then {
				_parsedValue = switch _writeType do {
					case "OVERWRITE": {
						_value;
					};

					case "ADD": {
						_oldValue + _value;
					};

					case "SUBTRACT": {
						_oldValue - _value;
					};

					case "MULTIPLY": {
						_oldValue * _value;
					};

					case "DIVIDE": {
						_oldValue / _value;
					};

					case "RAISE": {
						_oldValue ^ _value;
					};

					case "REMAINDER": {
						_oldValue % _value;
					};

					case "INTERSECT": {
						_oldValue arrayIntersect _value;
					};

					case "APPEND": {
						_oldValue append _value;
						_oldValue;
					};

					case "PUSHBACK": {
						_oldValue pushBack _value;
						_oldValue;
					};

					case "PUSHBACK_UNIQUE": {
						_oldValue pushBackUnique _value;
						_oldValue;
					};

					case "SET": {
						_oldValue set [_value select 0, _value select 1, false];
						_oldValue;
					};

					case "SET_UNIQUE": {
						_oldValue set [_value select 0, _value select 1, true];
						_oldValue;
					};

					case "INSERT": {
						_oldValue insert _value;
						_oldValue;
					};

					case "INSERT_UNIQUE": {
						_oldValue insert [_value select 0, _value select 1, true];
						_oldValue;
					};

					case "MERGE_OVERWRITE": {
						_oldValue merge [_value, true];
						_oldValue;
					};

					case "MERGE": {
						_oldValue merge [_value, false];
						_oldValue;
					};
				};
				
				if ((_parsedValue isEqualType createHashMap) || (_parsedValue isEqualType [])) then {
					_continue = false;
				};
			}
			else {
				if (_writeType isEqualTo "NIL") then {
					_continue = false;
				}
				else {
					_parsedValue = _value;
				};
			};

			if _continue then {
				_namespace setVariable [_parsedName, _parsedValue];
			};

			["KH_eve_variableChanged", [_namespace, _parsedName, _parsedValue, _oldValue, _writeType]] call CBA_fnc_localEvent;		
			private _handlerName = format ["KH_var_persistentPublicVariableState%1", _parsedName];
			_handler = ["PERSISTENT_HANDLER", _handlerName, _namespace];
			
			if (isNil {_namespace getVariable "KH_var_persistentPublicVariables"}) then {
				_namespace setVariable ["KH_var_persistentPublicVariables", createHashMap];
			};

			(_namespace getVariable ["KH_var_persistentPublicVariables", createHashMap]) set [_parsedName, _parsedValue];

			if (isNil {_namespace getVariable _handlerName}) then {
				_namespace setVariable [_handlerName, true];

				if !isServer then {
					_namespace setVariable [_parsedName, _parsedValue, 2];
				};

				[
					[_namespace, _parsedName, _handlerName, clientOwner], 
					{
						params ["_entity", "_parsedName", "_handlerName", "_currentOwner"];

						if (isNil {_entity getVariable _handlerName}) then {
							private _currentOwnerName = format ["KH_var_persistentVariableCurrentOwner%1", _parsedName];
							_entity setVariable ["KH_var_persistentPublicVariables", createHashMap];
							_entity setVariable [_currentOwnerName, _currentOwner];

							[
								[_entity, _parsedName, _handlerName, _currentOwner, _currentOwnerName], 
								{
									params ["_entity", "_parsedName", "_handlerName", "_currentOwner", "_currentOwnerName"];

									if (_entity getVariable [_handlerName, false]) then {
										[
											[_entity, _parsedName, _handlerName, _currentOwner, _currentOwnerName, clientOwner], 
											{
												params ["_entity", "_parsedName", "_handlerName", "_currentOwner", "_currentOwnerName", "_newOwner"];
												private _oldOwner = _entity getVariable [_currentOwner, 2];
												_entity setVariable [_currentOwnerName, _newOwner];

												if (_entity getVariable [_handlerName, false]) then {
													if (_oldOwner in KH_var_allMachines) then {
														[
															[_entity, _handlerName, _newOwner],
															{
																params ["_entity", "_handlerName", "_newOwner"];

																if (_entity getVariable [_handlerName, false]) then {
																	private _target = if !isServer then {
																		[2, _newOwner];
																	}
																	else {
																		_newOwner;
																	};

																	[
																		[_entity, _handlerName, _entity getVariable ["KH_var_persistentPublicVariables", createHashMap]], 
																		{
																			params ["_entity", "_handlerName", "_persistentVariables"];

																			if (_entity getVariable [_handlerName, false]) then {
																				{
																					_entity setVariable [_x, _y];
																				} forEach _persistentVariables;

																				_entity setVariable ["KH_var_persistentPublicVariables", _persistentVariables];
																			};										
																		},
																		_target,
																		true
																	] call KH_fnc_execute;
																};
															},
															_oldOwner,
															true
														] call KH_fnc_execute;
													}
													else {
														[
															[_entity, _handlerName, _entity getVariable ["KH_var_persistentPublicVariables", createHashMap]], 
															{
																params ["_entity", "_handlerName", "_persistentVariables"];

																if (_entity getVariable [_handlerName, false]) then {
																	{
																		_entity setVariable [_x, _y];
																	} forEach _persistentVariables;

																	_entity setVariable ["KH_var_persistentPublicVariables", _persistentVariables];
																};						
															},
															_newOwner,
															true
														] call KH_fnc_execute;
													};
												};										
											},
											"SERVER",
											true
										] call KH_fnc_execute;
									};
								},
								[
									"PERSISTENT", 
									_entity, 
									[], 
									{},
									true
								],
								true
							] call KH_fnc_execute;

							_entity setVariable [format ["KH_var_persistentPublicVariableState%1", _parsedName], true, true];
						};
					},
					"SERVER",
					true
				] call KH_fnc_execute;
			};
		};

		default {
			if ("JIP" in _scope) then {
				_handler = [
					[_namespace, _parsedName, _value, "LOCAL", _writeType],
					"KH_fnc_setVariable",
					["JIP", _scope select [4], true, false, false, format ["%1_%2", _namespace, _parsedName]],
					true
				] call KH_fnc_execute;
			}
			else {
				[
					[_namespace, _parsedName, _value, "LOCAL", _writeType],
					"KH_fnc_setVariable",
					_scope,
					true
				] call KH_fnc_execute;
			};
		};
	};
}
else {
	if (!(_namespace isEqualType "") && ((_scope isEqualTo true) || (_scope isEqualTo "LOCAL"))) exitWith {
		private _oldValue = _namespace getVariable _name;

		if !(isNil "_oldValue") then {
			_parsedValue = switch _writeType do {
				case "OVERWRITE": {
					_value;
				};

				case "ADD": {
					_oldValue + _value;
				};

				case "SUBTRACT": {
					_oldValue - _value;
				};

				case "MULTIPLY": {
					_oldValue * _value;
				};

				case "DIVIDE": {
					_oldValue / _value;
				};

				case "RAISE": {
					_oldValue ^ _value;
				};

				case "REMAINDER": {
					_oldValue % _value;
				};

				case "INTERSECT": {
					_oldValue arrayIntersect _value;
				};

				case "APPEND": {
					_oldValue append _value;
					_oldValue;
				};

				case "PUSHBACK": {
					_oldValue pushBack _value;
					_oldValue;
				};

				case "PUSHBACK_UNIQUE": {
					_oldValue pushBackUnique _value;
					_oldValue;
				};

				case "SET": {
					_oldValue set [_value select 0, _value select 1, false];
					_oldValue;
				};

				case "SET_UNIQUE": {
					_oldValue set [_value select 0, _value select 1, true];
					_oldValue;
				};

				case "INSERT": {
					_oldValue insert [_value select 0, _value select 1, false];
					_oldValue;
				};

				case "INSERT_UNIQUE": {
					_oldValue insert [_value select 0, _value select 1, true];
					_oldValue;
				};

				case "MERGE_OVERWRITE": {
					_oldValue merge [_value, true];
					_oldValue;
				};

				case "MERGE": {
					_oldValue merge [_value, false];
					_oldValue;
				};
			};
		}
		else {
			_parsedValue = _value;
		};

		_namespace setVariable [_name, _parsedValue];

		if (_scope isEqualTo true) then {
			[
				[_namespace, _name, _parsedValue],
				{
					params ["_namespace", "_name", "_parsedValue"];
					_namespace setVariable [_name, _parsedValue, true];
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;
		};

		_handler;
	};

	[
		[_namespace, _name, _value, "LOCAL", _writeType],
		"KH_fnc_setVariable",
		_scope,
		true,
		false
	] call KH_fnc_execute;
};

_handler;