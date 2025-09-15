params [["_type", "", ["", []]], ["_event", "", [true, 0, ""]], "_arguments", ["_function", {}, ["", {}]]];
private _argumentsId = call KH_fnc_generateUid;
missionNamespace setVariable [_argumentsId, _arguments];
private _eventNameId = call KH_fnc_generateUid;
missionNamespace setVariable [_eventNameId, _event];
private _handlerId = call KH_fnc_generateUid;
private _previousReturnId = call KH_fnc_generateUid;
private "_persistentEventId";
private "_persistentExecutionId";
private "_handler";
private "_remoteHandler";
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
				private _eventId = missionNamespace getVariable '", _handlerId, "';
				private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
				missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
			] joinString "");
		}
		else {
			_function = [_function, _scopeType isEqualTo "PERSISTENT"] call KH_fnc_parseFunction;
		};
	}
	else {
		_function = [_function, false] call KH_fnc_parseFunction;

		if ((_eventType isNotEqualTo "TEMPORAL") && (_eventType isNotEqualTo "DRAW_UI")) then {
			_expression = compile ([
				"private _args = missionNamespace getVariable '", _argumentsId, "';
				private _eventName = missionNamespace getVariable '", _eventNameId, "';
				private _eventId = missionNamespace getVariable '", _handlerId, "';
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
		private _eventId = missionNamespace getVariable '", _handlerId, "';
		private _previousReturn = missionNamespace getVariable '", _previousReturnId, "';
		missionNamespace setVariable ['", _previousReturnId, "', call (missionNamespace getVariable '", _function, "')];"
	] joinString "");
};

switch _eventType do {
	case "STANDARD": {
		private _entity = _type param [1, objNull, [objNull, grpNull]];
		private _scopeType = _type param [2, true, [true, ""]];

		switch _scopeType do {
			case true;
			case "LOCAL": {
				_handler = _entity addEventHandler [_event, _expression];
			};

			case "CURRENT_LOCAL": {
				_handler = _entity addEventHandler [_event, _expression];

				[
					[
						["STANDARD", _entity, "LOCAL"],
						"Local",
						[_event, _handler, clientOwner],
						{
							params ["_entity"];
							_args params ["_event", "_handler", "_eventOwner"];

							[
								[_entity, _event, _handler],
								{
									params ["_entity", "_event", "_handler"];
									_entity removeEventHandler [_event, _handler];
								},
								_eventOwner,
								true,
								false
							] call KH_fnc_execute;

							_entity removeEventHandler [_thisEvent, _thisEventHandler];
						}
					],
					"KH_fnc_addEventHandler",
					_entity,
					true,
					false
				] call KH_fnc_execute;
			};

			case "REMOTE": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistentExecutionId = call KH_fnc_generateUid;
				private _persistentEntityId = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntityId, _entity, true];
				private _remoteEventId = call KH_fnc_generateUid;
				private _entityOwnerId = call KH_fnc_generateUid;

				_remoteHandler = [
					"CBA",
					_remoteEventId,
					[_entityOwnerId, _argumentsId, _eventNameId, _function],
					{
						private _arguments = param [1];
						_args params ["_entityOwnerId", "_argumentsId", "_eventNameId", "_function"];
						private _entityOwner = missionNamespace getVariable _entityOwnerId;
						private _args = missionNamespace getVariable _argumentsId;
						private _eventName = missionNamespace getVariable _eventNameId;
						_arguments call (missionNamespace getVariable _function);
					}
				] call KH_fnc_addEventHandler;

				[
					"CBA",
					"KH_eve_eventHandlerRemoved",
					[_remoteHandler, _persistentExecutionId],
					{
						params ["_handler"];
						_args params ["_remoteHandler", "_persistentExecutionId"];

						if ((_handler select 1) isEqualTo _persistentExecutionId) then {
							[_remoteHandler select 1, _remoteHandler select 2] call CBA_fnc_removeEventHandler;
							["KH_eve_eventHandlerRemoved", _eventId select 2] call CBA_fnc_removeEventHandler;
						};
					}
				] call KH_fnc_addEventHandler;

				_handler = [
					[_entity, _event, _persistentEventId, _persistentExecutionId, _persistentEntityId, _entityOwnerId, clientOwner], 
					{
						params ["_entity", "_event", "_persistentEventId", "_persistentExecutionId", "_persistentEntityId", "_entityOwnerId", "_eventOwner"];
						missionNamespace setVariable [_persistentExecutionId, true];
						missionNamespace setVariable [_entityOwnerId, clientOwner, _eventOwner];

						missionNamespace setVariable [
							_persistentEventId, 
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistentExecutionId, "') then {
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
						true,
						[_entity, _persistentEventId, _persistentExecutionId], 
						{
							params ["_entity", "_persistentEventId", "_persistentExecutionId"];
							missionNamespace setVariable [_persistentExecutionId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						},
						""
					]
				] call KH_fnc_execute;
			};
			
			case "PERSISTENT": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistentExecutionId = call KH_fnc_generateUid;
				private _persistentEntityId = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntityId, _entity, true];

				_handler = [
					[_entity, _event, _arguments, _function, _argumentsId, _eventNameId, _handlerId, _previousReturnId, _persistentEventId, _persistentExecutionId, _persistentEntityId], 
					{
						params ["_entity", "_event", "_arguments", "_function", "_argumentsId", "_eventNameId", "_handlerId", "_previousReturnId", "_persistentEventId", "_persistentExecutionId", "_persistentEntityId"];
						missionNamespace setVariable [_persistentExecutionId, true];
						missionNamespace setVariable [_argumentsId, _arguments];
						missionNamespace setVariable [_eventNameId, _event];
						
						missionNamespace setVariable [
							_persistentEventId,
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistentExecutionId, "') then {
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
						true,
						[_entity, _persistentEventId, _persistentExecutionId], 
						{
							params ["_entity", "_persistentEventId", "_persistentExecutionId"];
							missionNamespace setVariable [_persistentExecutionId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						},
						""
					]
				] call KH_fnc_execute;
			};
		};
	};

	case "MULTIPLAYER": {
		private _entity = _type param [1, objNull, [objNull]];
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
		private _class = _type param [1, objNull, ["", objNull]];
		private _allowInheritance = _type param [2, true, [true]];
		private _excludedClasses = _type param [3, [], [[]]];
		private _applyRetroactively = _type param [4, true, [true]];

		if (_class isEqualType objNull) then {
			_class = typeOf _class;
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
		private _namespace = _type param [1, objNull, [true, objNull, grpNull, locationNull, displayNull, controlNull, missionNamespace]];
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
		private _iterationCount = false;
		private "_countConditionFailure";
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

		_timeoutRules params [["_timeout", 0, [true, 0, "", []]], ["_timeoutPriority", false, [true]], ["_timeoutOnConditionFailure", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		switch (typeName _timeout) do {
			case "BOOL": {
				if _timeout then {
					_timeout = 1;
					_iterationCount = true;
					_countConditionFailure = false;
					_handlerTickCounterId = call KH_fnc_generateUid;
					missionNamespace setVariable [_handlerTickCounterId, 1];
				}
				else {
					0;
				};
			};

			case "STRING": {
				_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
			};

			case "ARRAY": {
				_timeout = (_timeout select 0) max 1;
				_countConditionFailure = _timeout param [1, false, [true]];
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
						private _eventId = missionNamespace getVariable '", _handlerId, "';
						call (missionNamespace getVariable '", _timeoutFunction, "');"
					] joinString "");
				}
				else {
					{};
				}, 
				_handlerTickCounterId, 
				_timeout, 
				_timeoutOnDeletion
			]
		];

		private _continue = true;
		private "_previousReturn";

		if _immediate then {
			private _args = _arguments;
			private _totalDelta = 0;
			private _eventId = [_type, _event, _handler, clientOwner];
			private _eventName = _handler;
			private _executionTime = CBA_missionTime;
			private _executionCount = 0;

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
				private _conditionFunctionId = [_conditionFunction, false] call KH_fnc_parseFunction;

				if _iterationCount then {
					if _countConditionFailure then {
						if _timeoutOnConditionFailure then {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
								_previousReturn = _arguments call (missionNamespace getVariable _function);
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
							};
						}
						else {
							if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
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
							if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
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
							if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
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
						if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
							_previousReturn = _arguments call (missionNamespace getVariable _function);
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
						};
					}
					else {
						if (_conditionArguments call (missionNamespace getVariable _conditionFunctionId)) then {
							_previousReturn = _arguments call (missionNamespace getVariable _function);
						};
					};
				};
			};
		};

		if !_continue exitWith {};

		KH_var_temporalExecutionStackAdditions insert [
			[0, -1] select _timeoutPriority,
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
							missionNamespace getVariable _function;
						};
					}
					else {
						private _conditionFunctionId = [_conditionFunction, false] call KH_fnc_parseFunction;

						if _iterationCount then {
							if _countConditionFailure then {
								if _timeoutOnConditionFailure then {
									compile ([
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
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
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
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
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
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
										"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
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
									"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
										call (missionNamespace getVariable '", _function, "');
									}
									else {
										['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
									};"
								] joinString "");
							}
							else {
								compile ([
									"if ((missionNamespace getVariable '", _conditionArgumentsId, "') call (missionNamespace getVariable '", _conditionFunctionId, "')) then {
										call (missionNamespace getVariable '", _function, "');
									};"
								] joinString "");
							};
						};
					},
					_event,
					if (_event isEqualTo 0) then {
						diag_frameNo + 1;
					}
					else {
						if (_event > 0) then {
							diag_tickTime + _event;
						}
						else {
							diag_frameNo + (abs _event);
						};
					},
					if _verboseDelta then {
						systemTime joinString "";
					}
					else {
						-1;
					},
					[_type, _event, _handler, clientOwner],
					_handler,
					_previousReturn,
					CBA_missionTime,
					[0, 1] select _immediate
				]
			]
		];

		if (!_iterationCount && (_timeout isNotEqualTo 0)) then {
			private _timeoutId = call KH_fnc_generateUid;

			KH_var_temporalExecutionStackAdditions insert [
				[-1, 0] select _timeoutPriority,
				[
					[_handler],
					{
						params ["_handler"];
						["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
						KH_var_temporalExecutionStackDeletions pushBackUnique _eventId;
					},
					_timeout,
					if (_timeout isEqualTo 0) then {
						diag_frameNo + 1;
					}
					else {
						if (_timeout > 0) then {
							diag_tickTime + _timeout;
						}
						else {
							diag_frameNo + (abs _timeout);
						};
					},
					-1,
					_timeoutId,
					_timeoutId,
					nil,
					CBA_missionTime,
					0
				]
			];
		};
	};

	case "DRAW_UI": {
		private _conditionArguments = _type param [1];
		private _conditionFunction = _type param [2, {}, ["", {}]];
		private _timeoutRules = _type param [3, [0, false, false], [0, "", []]];
		private _timeoutArguments = _type param [4];
		private _timeoutFunction = _type param [5, {}, ["", {}]];
		private _conditionArgumentsId = call KH_fnc_generateUid;
		_timeoutFunction = [_timeoutFunction, false] call KH_fnc_parseFunction;
		missionNamespace setVariable [_conditionArgumentsId, _conditionArguments];
		_handler = call KH_fnc_generateUid;

		if (_timeoutRules isEqualTypeAny [0, ""]) then {
			_timeoutRules = [_timeoutRules, false, false, false];
		};

		_timeoutRules params [["_timeout", 0, [0, ""]], ["_timeoutOnConditionFailure", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		if (_timeout isEqualType "") then {
			_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
		};

		private _drawType = [KH_var_drawUi2dExecutionStackAdditions, KH_var_drawUi3dExecutionStackAdditions] select _event;

		_drawType pushBack [
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
			nil,
			CBA_missionTime
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
						private _eventId = missionNamespace getVariable '", _handlerId, "';
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
			private _timeoutId = call KH_fnc_generateUid;

			_drawType pushBack [
				[_handler],
				{
					params ["_handler"];
					["KH_eve_drawUiExecutionStackHandler", [_handler, true]] call CBA_fnc_localEvent;
				},
				_timeout,
				if (_timeout isEqualTo 0) then {
					diag_frameNo + 1;
				}
				else {
					if (_timeout > 0) then {
						diag_tickTime + _timeout;
					}
					else {
						diag_frameNo + (abs _timeout);
					};
				},
				-1,
				_timeoutId,
				_timeoutId,
				nil,
				CBA_missionTime
			]
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

						_handlerStack deleteAt (_handlerStackDeletions select {_x isEqualType 0;});
						_handlerStackDeletions resize 0;
					};

					if (_handlerStack isEqualTo []) then {
						inGameUISetEventHandler ['", _event, "', ''];
						_handlerStack deleteAt ", _event, ";
					}
					else {
						{
							if !((_x select 0) in _handlerStackDeletions) then {
								call (_x select 1);
							};
						} forEach _handlerStack;
					};"
				] joinString ""
			];
		};

		_handler = [call KH_fnc_generateUid, _expression];
		_currentStack pushBack _handler;
	};

	case "PLAYER": {
		_handler = [_event, _expression] call CBA_fnc_addPlayerEventHandler;
	};

	case "CBA": {
		_handler = [_event, _expression] call CBA_fnc_addEventHandler;
	};
};

if !(_type isEqualType []) then {
	_type = [_type];
};

if (isNil "_persistentEventId") then {
	missionNamespace setVariable [_handlerId, [_type, _event, _handler, clientOwner]];
	["KH_eve_eventHandlerAdded", [[_type, _event, _handler, clientOwner]]] call CBA_fnc_localEvent;
	[_type, _event, _handler, clientOwner];
}
else {
	private "_eventOwner";
	
	if !(isNil "_remoteHandler") then {
		_eventOwner = clientOwner;
	};

	missionNamespace setVariable [_handlerId, [_type, _event, _handler, _persistentExecutionId, _eventOwner], true];
	["KH_eve_eventHandlerAdded", [[_type, _event, _handler, _persistentExecutionId, _eventOwner]]] call CBA_fnc_globalEvent;
	[_type, _event, _handler, _persistentExecutionId, _eventOwner];
};