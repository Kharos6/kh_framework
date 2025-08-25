params [["_type", "", ["", []]], ["_event", "", ["", 0]], "_arguments", ["_function", {}, ["", {}]]];
private _argumentsId = call KH_fnc_generateUid;
missionNamespace setVariable [_argumentsId, _arguments];
private _eventNameId = call KH_fnc_generateUid;
missionNamespace setVariable [_eventNameId, _event];
private _handlerId = call KH_fnc_generateUid;
private _previousReturnId = call KH_fnc_generateUid;
private "_persistentEventId";
private "_persistencyId";
private "_handler";
private "_eventType";
private "_expression";

if (_type isEqualType []) then {
	_eventType = _type param [0, "", [""]];

	if (_eventType isEqualTo "STANDARD") then {
		private _scopeType = _type param [2, true, [true, ""]];

		if ((_scopeType isNotEqualTo "REMOTE") && (_scopeType isNotEqualTo "PERSISTENT")) then {
			_function = [_function, false] call KH_fnc_parseFunction;

			_expression = compile ([
				"private _args = missionNamespace getVariable '", _argumentsId, "';
				private _eventName = missionNamespace getVariable '", _eventNameId, "';
				private _localId = missionNamespace getVariable '", _handlerId, "';
				private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
				missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
			] joinString "");
		}
		else {
			_function = [_function, true] call KH_fnc_parseFunction;
		};
	}
	else {
		_function = [_function, false] call KH_fnc_parseFunction;

		if ((_eventType isNotEqualTo "TEMPORAL") && (_eventType isNotEqualTo "DRAW_UI")) then {
			_expression = compile ([
				"private _args = missionNamespace getVariable '", _argumentsId, "';
				private _eventName = missionNamespace getVariable '", _eventNameId, "';
				private _localId = missionNamespace getVariable '", _handlerId, "';
				private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
				missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
			] joinString "");
		};
	};
}
else {
	_function = [_function, false] call KH_fnc_parseFunction;
	_eventType = _type;

	_expression = compile ([
		"private _args = missionNamespace getVariable '", _argumentsId, "';
		private _eventName = missionNamespace getVariable '", _eventNameId, "';
		private _localId = missionNamespace getVariable '", _handlerId, "';
		private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
		missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
	] joinString "");
};

switch _eventType do {
	case "STANDARD": {
		private _entity = _type param [1, objNull, [objNull, teamMemberNull, grpNull]];
		private _scopeType = _type param [2, true, [true, ""]];

		if (_entity isEqualType teamMemberNull) then {
			_entity = agent _entity;
		};

		switch _scopeType do {
			case true;
			case "LOCAL": {
				_handler = _entity addEventHandler [_event, _expression];
			};

			case "REMOTE": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistencyId = call KH_fnc_generateUid;
				private _persistentEntityId = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntityId, _entity, true];
				private _remoteEventId = call KH_fnc_generateUid;
				private _entityOwnerId = call KH_fnc_generateUid;

				[
					_remoteEventId, 
					compile ([
						"private _entityOwner = missionNamespace getVariable '", _entityOwnerId, "';
						private _args = missionNamespace getVariable '", _argumentsId, "';
						private _eventName = missionNamespace getVariable '", _eventNameId, "';
						private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
						missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
					] joinString "")
				] call CBA_fnc_addEventHandler;

				_handler = [
					[_entity, _event, _persistentEventId, _persistencyId, _persistentEntityId, _entityOwnerId, clientOwner], 
					{
						params ["_entity", "_event", "_persistentEventId", "_persistencyId", "_persistentEntityId", "_entityOwnerId", "_eventOwner"];
						missionNamespace setVariable [_persistencyId, true];
						missionNamespace setVariable [_entityOwnerId, clientOwner, _eventOwner];

						missionNamespace setVariable [
							_persistentEventId, 
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistencyId, "') then {
										['", _remoteEventId, "', _this, ", _eventOwner, "] call CBA_fnc_ownerEvent;
									}
									else {
										(missionNamespace getVariable ['", _persistentEntityId, "', objNull]) removeEventHandler [_thisEvent, _thisEventHandler];
									};"
								] joinString "")
							]
						];
					},
					_entity,
					true,
					[
						"PERSISTENT",
						[_entity, _persistentEventId, _persistencyId], 
						{
							params ["_entity", "_persistentEventId", "_persistencyId"];
							missionNamespace setVariable [_persistencyId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						},
						""
					]
				] call KH_fnc_execute;
			};
			
			case "PERSISTENT": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistencyId = call KH_fnc_generateUid;
				private _persistentEntityId = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntityId, _entity, true];

				_handler = [
					[_entity, _event, _arguments, _function, _argumentsId, _eventNameId, _handlerId, _previousReturnId, _persistentEventId, _persistencyId, _persistentEntityId], 
					{
						params ["_entity", "_event", "_arguments", "_function", "_argumentsId", "_eventNameId", "_handlerId", "_previousReturnId", "_persistentEventId", "_persistencyId", "_persistentEntityId"];
						missionNamespace setVariable [_persistencyId, true];
						missionNamespace setVariable [_argumentsId, _arguments];
						missionNamespace setVariable [_eventNameId, _event];
						
						missionNamespace setVariable [
							_persistentEventId,
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistencyId, "') then {
										private _args = missionNamespace getVariable '", _argumentsId, "';
										private _eventName = missionNamespace getVariable '", _eventNameId, "';
										private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
										missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];
									}
									else {
										(missionNamespace getVariable ['", _persistentEntityId, "', objNull]) removeEventHandler [_thisEvent, _thisEventHandler];
									};"
								] joinString "")
							]
						];
					},
					_entity,
					true,
					[
						"PERSISTENT",
						[_entity, _persistentEventId, _persistencyId], 
						{
							params ["_entity", "_persistentEventId", "_persistencyId"];
							missionNamespace setVariable [_persistencyId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						},
						""
					]
				] call KH_fnc_execute;
			};
		};
	};

	case "MULTIPLAYER": {
		private _entity = _type param [1, objNull, [objNull, teamMemberNull]];

		if (_entity isEqualType teamMemberNull) then {
			_entity = agent _entity;
		};

		_handler = _entity addMPEventHandler [_event, _expression];
	};

	case "CONTROL": {
		private _control = _type param [1, controlNull, [controlNull]];
		_handler = _control ctrlAddEventHandler [_event, _expression];
	};

	case "DISPLAY": {
		private _display = _type param [1, displayNull, [displayNull]];
		_handler = _display displayAddEventHandler [_event, _expression];
	};

	case "PUBLIC_VARIABLE": {
		_handler = call KH_fnc_generateUid;
		missionNamespace setVariable [_handler, true];

		_event addPublicVariableEventHandler (compile ([
			"if (missionNamespace getVariable '", _handler, "') then {
				call ", _expression,
			"};"
		] joinString ""));
	};

	case "CLASS": {
		private _class = _type param [1, objNull, ["", objNull, teamMemberNull]];
		private _allowInheritance = _type param [2, true, [true]];
		private _excludedClasses = _type param [3, [], [[]]];
		private _applyRetroactively = _type param [4, true, [true]];

		switch (typeName _class) do {
			case "OBJECT": {
				_class = typeOf _class;
			};

			case "TEAM_MEMBER": {
				_class = typeOf (agent _class);
			};
		};

		_handler = call KH_fnc_generateUid;
		missionNamespace setVariable [_handler, true];

		[
			_class,
			_event, 
			compile ([
				"if (missionNamespace getVariable '", _handler, "') then {
					call ", _expression,
				"};"
			] joinString ""),
			_allowInheritance,
			_excludedClasses,
			_applyRetroactively
		] call CBA_fnc_addClassEventHandler;
	};

	case "MISSION": {
		_handler = addMissionEventHandler [_event, _expression];
	};

	case "USER_ACTION": {
		_handler = addUserActionEventHandler [_event, _expression];
	};

	case "MUSIC": {
		_handler = addMusicEventHandler [_event, _expression];
	};

	case "BIS_SCRIPTED": {
		private _namespace = _type param [1, objNull, [true, objNull, teamMemberNull, grpNull, locationNull, displayNull, controlNull, missionNamespace]];

		if (_namespace isEqualType teamMemberNull) then {
			_namespace = agent _namespace;
		};

		_handler = [_namespace, _event, _expression] call BIS_fnc_addScriptedEventHandler;
	};

	case "TEMPORAL": {
		private _immediate = _type param [1, true, [true]];
		private _conditionArguments = _type param [2];
		private _conditionFunction = _type param [3, {}, ["", {}]];
		private _timeoutRules = _type param [4, [0, false, false, false], [true, 0, "", []]];
		private _timeoutArguments = _type param [5];
		private _timeoutFunction = _type param [6, {}, ["", {}]];
		private _verboseDelta = _type param [7, false, [true]];
		private _conditionArgumentsId = call KH_fnc_generateUid;
		private _handlerTickCounterId = call KH_fnc_generateUid;
		private _countConditionFailure = false;
		private _iterationCount = false;
		_timeoutFunction = [_timeoutFunction, false] call KH_fnc_parseFunction;
		missionNamespace setVariable [_conditionArgumentsId, _conditionArguments];
		_handler = call KH_fnc_generateUid;

		switch (typeName _timeoutRules) do {
			case "BOOL": {
				if _timeoutRules then {
					_timeoutRules = [[1], false, false, false];
				}
				else {
					_timeoutRules = [0, false, false, false];
				};
			};

			case "SCALAR";
			case "STRING": {
				_timeoutRules = [_timeoutRules, false, false, false];
			};
		};

		_timeoutRules params [["_timeout", 0, [0, "", []]], ["_timeoutPriority", false, [true]], ["_timeoutOnConditionFailure", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		switch (typeName _timeout) do {
			case "STRING": {
				_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
			};

			case "ARRAY": {
				_countConditionFailure = _timeout param [1, false, [true]];
				_timeout = (_timeout select 0) max 1;
				_iterationCount = true;
				_handlerTickCounterId = call KH_fnc_generateUid;
				missionNamespace setVariable [_handlerTickCounterId, 1];
			};
		};

		KH_var_temporalExecutionStackMonitor set [
			_handler, 
			[
				_timeoutArguments, 
				if ((missionNamespace getVariable _timeoutFunction) isNotEqualTo {}) then {
					compile ([
						"private _args = missionNamespace getVariable '", _argumentsId, "';
						private _eventName = missionNamespace getVariable '", _eventNameId, "';
						private _localId = missionNamespace getVariable '", _handlerId, "';
						call (missionNamespace getVariable '", _timeoutFunction, "');"
					] joinString "");
				}
				else {
					{};
				}, 
				_handlerTickCounterId, 
				_timeout, 
				_timeoutPriority, 
				_timeoutOnDeletion
			]
		];

		private _continue = true;
		private "_previousReturn";

		if _immediate then {
			private _args = _arguments;
			private _totalDelta = 0;
			private _localId = [_type, _event, _handler, clientOwner];
			private _eventName = _handler;

			if ((_conditionFunction isEqualTo {}) || (_conditionFunction isEqualTo "")) then {						
				if _iterationCount then {
					_previousReturn = _arguments call (missionNamespace getVariable _function);
					["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

					if (_timeout isEqualTo 1) then {
						_continue = false;
					};
				}
				else {
					_previousReturn = _arguments call (missionNamespace getVariable _function);
				};
			}
			else {
				_conditionFunction = [_conditionFunction, false] call KH_fnc_parseFunction;
				
				if _iterationCount then {
					if _countConditionFailure then {
						if _timeoutOnConditionFailure then {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
								_previousReturn = _arguments call (missionNamespace getVariable _function);
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
							};
						}
						else {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
								_previousReturn = _arguments call (missionNamespace getVariable _function);
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							};
						};

						if (_timeout isEqualTo 1) then {
							_continue = false;
						};
					}
					else {
						if _timeoutOnConditionFailure then {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
								_previousReturn = _arguments call (missionNamespace getVariable _function);
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

								if (_timeout isEqualTo 1) then {
									_continue = false;
								};
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
							};
						}
						else {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
								_previousReturn = _arguments call (missionNamespace getVariable _function);
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

								if (_timeout isEqualTo 1) then {
									_continue = false;
								};
							};
						};
					};
				}
				else {
					if _timeoutOnConditionFailure then {
						if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
							_previousReturn = _arguments call (missionNamespace getVariable _function);
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
						};
					}
					else {
						if (_conditionArguments call (missionNamespace getVariable _conditionFunction)) then {
							_previousReturn = _arguments call (missionNamespace getVariable _function);
						};
					};
				};
			};
		};

		if !_continue exitWith {};

		KH_var_temporalExecutionStackAdditions insert [
			if _timeoutPriority then {
				-1;
			}
			else {
				0;
			},
			[
				[
					_arguments,
					if ((_conditionFunction isEqualTo {}) || (_conditionFunction isEqualTo "")) then {						
						if _iterationCount then {
							compile ([
								"call (missionNamespace getVariable '", _function, "');
								['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;"
							] joinString "");
						}
						else {
							_function;
						};
					}
					else {						
						if _iterationCount then {
							if _countConditionFailure then {
								if _timeoutOnConditionFailure then {
									compile ([
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
											call (missionNamespace getVariable '", _function, "');
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										}
										else {
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								}
								else {
									compile ([
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
											call (missionNamespace getVariable '", _function, "');
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										}
										else {
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								};
							}
							else {
								if _timeoutOnConditionFailure then {
									compile ([
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
											call (missionNamespace getVariable '", _function, "');
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										}
										else {
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								}
								else {
									compile ([
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
											call (missionNamespace getVariable '", _function, "');
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								};
							};
						}
						else {
							if _timeoutOnConditionFailure then {
								compile ([
									"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
										call (missionNamespace getVariable '", _function, "');
									}
									else {
										['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
									};"
								] joinString "");
							}
							else {
								compile ([
									"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
										call (missionNamespace getVariable '", _function, "');
									};"
								] joinString "");
							};
						};
					},
					_event,
					if (_event >= 0) then {
						diag_tickTime + _event;
					}
					else {
						diag_frameNo + (abs _event);
					},
					if _verboseDelta then {
						systemTime joinString "";
					}
					else {
						-1;
					},
					[_type, _event, _handler, clientOwner],
					_handler,
					_previousReturn
				]
			]
		];

		if (!_iterationCount && (_timeout isNotEqualTo 0)) then {
			[
				[
					"TEMPORAL",
					false,
					[],
					{},
					[[1, false], !_timeoutPriority, false, false],
					[],
					{},
					false
				],
				_timeout,
				[_handler],
				{
					params ["_handler"];
					["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
				}
			] call KH_fnc_addEventHandler;
		};
	};

	case "DRAW_UI": {
		private _conditionArguments = _type param [1];
		private _conditionFunction = _type param [2, {}, ["", {}]];
		private _timeoutRules = _type param [3, [0, false, false], [[]]];
		private _timeoutArguments = _type param [4];
		private _timeoutFunction = _type param [5, {}, ["", {}]];
		private _conditionArgumentsId = call KH_fnc_generateUid;
		_timeoutFunction = [_timeoutFunction, false] call KH_fnc_parseFunction;
		missionNamespace setVariable [_conditionArgumentsId, _conditionArguments];
		_handler = call KH_fnc_generateUid;

		if ((_timeoutRules isEqualType 0) || (_timeoutRules isEqualType "")) then {
			_timeoutRules = [_timeoutRules, false, false, false];
		};

		_timeoutRules params [["_timeout", 0, [0, ""]], ["_timeoutOnConditionFailure", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		if (_timeout isEqualType "") then {
			_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
		};

		private _drawType = switch _event do {
			case "2D": {
				KH_var_drawUi2dExecutionStackAdditions;
			};

			case "3D": {
				KH_var_drawUi3dExecutionStackAdditions;
			};
		};

		_drawType pushBack [
			[
				_arguments,
				if ((_conditionFunction isEqualTo {}) || (_conditionFunction isEqualTo "")) then {						
					missionNamespace getVariable _function;
				}
				else {
					_conditionFunction = [_conditionFunction, false] call KH_fnc_parseFunction;
					
					if _timeoutOnConditionFailure then {
						compile ([
							"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
								call (missionNamespace getVariable '", _function, "');
							}
							else {
								['KH_eve_drawUiExecutionStackHandler', ['", _handler, "', true]] call CBA_fnc_localEvent;
							};"
						] joinString "");
					}
					else {
						compile ([
							"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunction, "')) then {
								call (missionNamespace getVariable '", _function, "');
							};"
						] joinString "");
					};
				},
				[_type, _event, _handler, clientOwner],
				_handler,
				nil
			]
		];

		KH_var_drawUiExecutionStackMonitor set [
			_handler, 
			[
				_event, 
				_timeoutArguments,
				if ((missionNamespace getVariable _timeoutFunction) isNotEqualTo {}) then {
					compile ([
						"private _args = missionNamespace getVariable '", _argumentsId, "';
						private _eventName = missionNamespace getVariable '", _eventNameId, "';
						private _localId = missionNamespace getVariable '", _handlerId, "';
						call (missionNamespace getVariable '", _timeoutFunction, "');"
					] joinString "");
				}
				else {
					{};
				}, 
				_timeoutOnDeletion
			]
		];

		if (_timeout isNotEqualTo 0) then {
			[
				[
					"TEMPORAL",
					false,
					[],
					{},
					[[1, false], false, false, false],
					[],
					{},
					false
				],
				_timeout,
				[_handler],
				{
					params ["_handler"];
					["KH_eve_drawUiExecutionStackHandler", [_handler, true]] call CBA_fnc_localEvent;
				}
			] call KH_fnc_addEventHandler;
		};
	};

	case "IN_GAME_UI": {
		private _currentStack = KH_var_inGameUiEventHandlerStack get _event;

		if (isNil "_currentStack") then {
			KH_var_inGameUiEventHandlerStack set [_event, []];
			_currentStack = KH_var_inGameUiEventHandlerStack get _event;

			inGameUISetEventHandler [
				_event,
				[
					"private _handlerStack = (missionNamespace getVariable 'KH_var_inGameUiEventHandlerStack') get '", _event, "';
					private _handlerStackDeletions = missionNamespace getVariable 'KH_var_inGameUiEventHandlerStackDeletions';
					
					if (_handlerStackDeletions isNotEqualTo []) then {
						{
							if ((_x select 0) in _handlerStackDeletions) then {
								_handlerStackDeletions set [_handlerStack find (_x select 0), _forEachIndex];
							};
						} forEach _handlerStack;

						_handlerStack deleteAt (_handlerStackDeletions select {_x isEqualType 0});
						_handlerStackDeletions = [];
					};

					if (_handlerStack isEqualTo []) exitWith {
						inGameUISetEventHandler ['", _event, "', ''];
						_handlerStack deleteAt ", _event, ";
					};

					{
						if !((_x select 0) in _handlerStackDeletions) then {
							call (_x select 1);
						};
					} forEach _handlerStack;"
				] joinString ""
			];
		};

		_handler = [call KH_fnc_generateUid, _expression];
		_currentStack pushBack _handler;
	};

	case "PLAYER": {
		private _currentStack = KH_var_playerEventHandlerStack get _event;

		if (isNil "_currentStack") then {
			KH_var_playerEventHandlerStack set [_event, []];
			_currentStack = KH_var_playerEventHandlerStack get _event;

			[
				_event,
				compile ([
					"private _handlerStack = (missionNamespace getVariable 'KH_var_playerEventHandlerStack') get '", _event, "';
					private _handlerStackDeletions = missionNamespace getVariable 'KH_var_playerEventHandlerStackDeletions';
					
					if (_handlerStackDeletions isNotEqualTo []) then {
						{
							if ((_x select 0) in _handlerStackDeletions) then {
								_handlerStackDeletions set [_handlerStack find (_x select 0), _forEachIndex];
							};
						} forEach _handlerStack;

						_handlerStack deleteAt (_handlerStackDeletions select {_x isEqualType 0});
						_handlerStackDeletions = [];
					};

					{
						if !((_x select 0) in _handlerStackDeletions) then {
							call (_x select 1);
						};
					} forEach _handlerStack;"
				] joinString ""),
				_type select 1
			] call CBA_fnc_addPlayerEventHandler;
		};

		_handler = [call KH_fnc_generateUid, _expression];
		_currentStack pushBack _handler;
	};

	case "CBA": {
		private _currentStack = KH_var_cbaEventHandlerStack get _event;

		if (isNil "_currentStack") then {
			KH_var_cbaEventHandlerStack set [_event, []];
			_currentStack = KH_var_cbaEventHandlerStack get _event;

			[
				_event,
				compile ([
					"private _handlerStack = (missionNamespace getVariable 'KH_var_cbaEventHandlerStack') get '", _event, "';
					private _handlerStackDeletions = missionNamespace getVariable 'KH_var_cbaEventHandlerStackDeletions';
					
					if (_handlerStackDeletions isNotEqualTo []) then {
						{
							if ((_x select 0) in _handlerStackDeletions) then {
								_handlerStackDeletions set [_handlerStack find (_x select 0), _forEachIndex];
							};
						} forEach _handlerStack;

						_handlerStack deleteAt (_handlerStackDeletions select {_x isEqualType 0});
						_handlerStackDeletions = [];
					};

					{
						if !((_x select 0) in _handlerStackDeletions) then {
							call (_x select 1);
						};
					} forEach _handlerStack;"
				] joinString "")
			] call CBA_fnc_addEventHandler;
		};

		_handler = [call KH_fnc_generateUid, _expression];
		_currentStack pushBack _handler;
	};
};

if (isNil "_persistentEventId") then {
	if !(_type isEqualType []) then {
		_type = [_type];
	};

	missionNamespace setVariable [_handlerId, [_type, _event, _handler, clientOwner]];
	[_type, _event, _handler, clientOwner];
}
else {
	missionNamespace setVariable [_handlerId, [_handler, _persistencyId], true];	
	[_handler, _persistencyId];
};