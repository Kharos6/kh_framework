params [["_type", "", ["", []]], ["_event", "", ["", 0]], "_arguments", ["_function", {}, ["", {}]]];
private _handlerArguments = call KH_fnc_generateUid;
missionNamespace setVariable [_handlerArguments, _arguments];
private _handlerId = call KH_fnc_generateUid;
missionNamespace setVariable [_handlerId, -1];
private _eventName = call KH_fnc_generateUid;
missionNamespace setVariable [_eventName, _event];
private _previousReturn = call KH_fnc_generateUid;
private "_persistentEventId";
private "_persistentEntityId";
private "_handler";
private "_eventType";
private "_expression";

if (_type isEqualType []) then {
	_eventType = _type select 0;

	if (_eventType isEqualTo "STANDARD") then {
		private _scopeType = _type select 2;

		if ((_scopeType isNotEqualTo "REMOTE") && (_scopeType isNotEqualTo "PERSISTENT")) then {
			_function = [_function, false] call KH_fnc_parseFunction;

			_expression = compile ([
				"private _args = missionNamespace getVariable '", _handlerArguments, "';
				private _eventName = missionNamespace getVariable '", _eventName, "';
				private _localId = missionNamespace getVariable '", _handlerId, "';
				private _previousReturn = missionNamespace getVariable '", _previousReturn, "';
				missionNamespace setVariable ['", _previousReturn, "', call (missionNamespace getVariable '", _function, "')];"
			] joinString "");
		}
		else {
			_function = [_function, true] call KH_fnc_parseFunction;
		};
	}
	else {
		_function = [_function, false] call KH_fnc_parseFunction;

		if (_eventType isNotEqualTo "TEMPORAL") then {
			_expression = compile ([
				"private _args = missionNamespace getVariable '", _handlerArguments, "';
				private _eventName = missionNamespace getVariable '", _eventName, "';
				private _localId = missionNamespace getVariable '", _handlerId, "';
				private _previousReturn = missionNamespace getVariable '", _previousReturn, "';
				missionNamespace setVariable ['", _previousReturn, "', call (missionNamespace getVariable '", _function, "')];"
			] joinString "");
		};
	};
}
else {
	_function = [_function, false] call KH_fnc_parseFunction;
	_eventType = _type;

	_expression = compile ([
		"private _args = missionNamespace getVariable '", _handlerArguments, "';
		private _eventName = missionNamespace getVariable '", _eventName, "';
		private _localId = missionNamespace getVariable '", _handlerId, "';
		private _previousReturn = missionNamespace getVariable '", _previousReturn, "';
		missionNamespace setVariable ['", _previousReturn, "', call (missionNamespace getVariable '", _function, "')];"
	] joinString "");
};

switch _eventType do {
	case "STANDARD": {
		private _entity = _type param [1, objNull, ["", objNull, teamMemberNull, grpNull]];
		private _scopeType = _type param [2, "LOCAL", [""]];

		switch (typeName _entity) do {
			case "STRING": {
				_entity = missionNamespace getVariable _entity;
			};

			case "TEAM_MEMBER": {
				_entity = agent _entity;
			};
		};

		switch _scopeType do {
			case "LOCAL": {
				_handler = _entity addEventHandler [_event, _expression];
			};

			case "REMOTE": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistentEntityId = call KH_fnc_generateUid;
				private _persistentEntity = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntity, _entity];

				[
					[_persistentEntity, _entity],
					{
						params ["_persistentEntity", "_entity"];
						missionNamespace setVariable [_persistentEntity, _entity, true];
					},
					"SERVER",
					true
				] call KH_fnc_execute;

				private _remoteEventId = call KH_fnc_generateUid;
				private _entityOwnerId = call KH_fnc_generateUid;

				[
					_remoteEventId, 
					compile ([
						"private _entityOwner = missionNamespace getVariable '", _entityOwnerId, "';
						call ", _expression, ";"
					] joinString "")
				] call CBA_fnc_addEventHandler;

				_handler = [
					[_entity, _event, _persistentEventId, _persistentEntityId, _persistentEntity, _entityOwnerId, clientOwner], 
					{
						params ["_entity", "_event", "_persistentEventId", "_persistentEntityId", "_persistentEntity", "_entityOwnerId", "_eventOwner"];
						missionNamespace setVariable [_persistentEntityId, true];
						missionNamespace setVariable [_entityOwnerId, clientOwner, _eventOwner];

						missionNamespace setVariable [
							_persistentEventId, 
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistentEntityId, "') then {
										['", _remoteEventId, "', _this, ", _eventOwner, "] call CBA_fnc_ownerEvent;
									}
									else {
										(missionNamespace getVariable ['", _persistentEntity, "', objNull]) removeEventHandler [_thisEvent, _thisEventHandler];
									};"
								] joinString "")
							]
						];
					},
					[
						"PERSISTENT", 
						_entity, 
						[_entity, _persistentEventId, _persistentEntityId], 
						{
							params ["_entity", "_persistentEventId", "_persistentEntityId"];
							missionNamespace setVariable [_persistentEntityId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						}, 
						true
					],
					true
				] call KH_fnc_execute;
			};
			
			case "PERSISTENT": {
				_persistentEventId = call KH_fnc_generateUid;
				_persistentEntityId = call KH_fnc_generateUid;
				private _persistentEntity = call KH_fnc_generateUid;
				missionNamespace setVariable [_persistentEntity, _entity];

				[
					[_persistentEntity, _entity],
					{
						params ["_persistentEntity", "_entity"];
						missionNamespace setVariable [_persistentEntity, _entity, true];
					},
					"SERVER",
					true
				] call KH_fnc_execute;

				_handler = [
					[_entity, _event, _arguments, _function, _handlerArguments, _eventName, _handlerId, _previousReturn, _persistentEventId, _persistentEntityId, _persistentEntity], 
					{
						params ["_entity", "_event", "_arguments", "_function", "_handlerArguments", "_eventName", "_handlerId", "_previousReturn", "_persistentEventId", "_persistentEntityId", "_persistentEntity"];
						missionNamespace setVariable [_persistentEntityId, true];
						missionNamespace setVariable [_handlerArguments, _arguments];
						missionNamespace setVariable [_eventName, _event];
						
						missionNamespace setVariable [
							_persistentEventId,
							_entity addEventHandler [
								_event, 
								compile ([
									"if (missionNamespace getVariable '", _persistentEntityId, "') then {
										private _args = missionNamespace getVariable '", _handlerArguments, "';
										private _eventName = missionNamespace getVariable '", _eventName, "';
										private _previousReturn = missionNamespace getVariable '", _previousReturn, "';
										missionNamespace setVariable ['", _previousReturn, "', call (missionNamespace getVariable '", _function, "')];
									}
									else {
										(missionNamespace getVariable ['", _persistentEntity, "', objNull]) removeEventHandler [_thisEvent, _thisEventHandler];
									};"
								] joinString "")
							]
						];
					},
					[
						"PERSISTENT", 
						_entity, 
						[_entity, _persistentEventId, _persistentEntityId], 
						{
							params ["_entity", "_persistentEventId", "_persistentEntityId"];
							missionNamespace setVariable [_persistentEntityId, false];
							_entity removeEventHandler (missionNamespace getVariable [_persistentEventId, []]);
						}, 
						true
					],
					true
				] call KH_fnc_execute;
			};
		};
	};

	case "MULTIPLAYER": {
		private _entity = _type param [1, objNull, ["", objNull, teamMemberNull]];

		switch (typeName _entity) do {
			case "STRING": {
				_entity = missionNamespace getVariable _entity;
			};

			case "TEAM_MEMBER": {
				_entity = agent _entity;
			};
		};

		_handler = _entity addMPEventHandler [_event, _expression];
	};

	case "CONTROL": {
		_handler = (_type select 1) ctrlAddEventHandler [_event, _expression];
	};

	case "DISPLAY": {
		_handler = (_type select 1) displayAddEventHandler [_event, _expression];
	};

	case "CLASS": {
		private _entity = _type param [1, objNull, ["", objNull, teamMemberNull]];

		switch (typeName _entity) do {
			case "OBJECT": {
				_entity = typeOf _entity;
			};

			case "TEAM_MEMBER": {
				_entity = typeOf (agent _entity);
			};
		};

		_handler = call KH_fnc_generateUid;

		[
			_type select 1,
			_event, 
			compile ([
				"if (missionNamespace getVariable ['", _handler, "', true]) then {
					call ", _expression,
				"};"
			] joinString ""),
			_type select 2,
			_type select 3,
			_type select 4
		] call CBA_fnc_addClassEventHandler;
	};

	case "PLAYER": {
		private _currentStack = KH_var_cbaPlayerEventHandlerStack get _event;

		if (isNil "_currentStack") then {
			KH_var_cbaPlayerEventHandlerStack set [_event, []];
			_currentStack = KH_var_cbaPlayerEventHandlerStack get _event;

			[
				_event,
				compile ([
					"private _handlerStack = (missionNamespace getVariable 'KH_var_cbaPlayerEventHandlerStack') get '", _event, "';
					private _handlerStackDeletions = missionNamespace getVariable 'KH_var_cbaPlayerEventHandlerStackDeletions';
					
					if (_handlerStackDeletions isNotEqualTo []) then {
						{
							if ((_x select 0) in _handlerStackDeletions) then {
								_handlerStackDeletions set [_handlerStack find (_x select 0), _forEachIndex];
							};
						} forEach _handlerStack;

						{
							if (_x isEqualType 0) then {
								_handlerStack deleteAt _x;
							};
						} forEach _handlerStackDeletions;

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

	case "PUBLIC_VARIABLE": {
		_handler = call KH_fnc_generateUid;
		missionNamespace setVariable [_handler, true];

		_event addPublicVariableEventHandler (compile ([
			"if (missionNamespace getVariable '", _handler, "') then {
				call ", _expression,
			"};"
		] joinString ""));
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
		_handler = [_type select 1, _event, _expression] call BIS_fnc_addScriptedEventHandler;
	};

	case "TEMPORAL": {
		private _immediate = _type param [1, true, [true]];
		private _conditionArguments = _type param [2, nil];
		private _conditionFunction = _type param [3, {}, ["", {}]];
		private _timeoutRules = _type param [4, [0, false, false, false], ["", []]];
		private _timeoutArguments = _type param [5, []];
		private _timeoutFunction = _type param [6, {}, ["", {}]];
		private _simpleDelta = _type param [7, true, [true]];
		private _handlerConditionArguments = call KH_fnc_generateUid;
		private _handlerTickCounter = call KH_fnc_generateUid;
		private _countConditionFailure = false;
		private _iterationCount = false;

		if !(_timeoutFunction isEqualType {}) then {
			_timeoutFunction = [[_timeoutFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
		};

		missionNamespace setVariable [_handlerConditionArguments, _conditionArguments];
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
				_handlerTickCounter = call KH_fnc_generateUid;
				missionNamespace setVariable [_handlerTickCounter, 1];
			};
		};

		KH_var_temporalExecutionStackMonitor set [
			_handler, 
			[
				_timeoutArguments, 
				if (_timeoutFunction isNotEqualTo {}) then {
					compile ([
						"private _args = missionNamespace getVariable ['", _handlerArguments, "', []];
						private _eventName = missionNamespace getVariable ['", _eventName, "', ''];
						private _localId = missionNamespace getVariable ['", _handlerId, "', []];
						call ", _timeoutFunction, ";"
					] joinString "")
				}
				else {
					{};
				}, 
				_handlerTickCounter, 
				_timeout, 
				_timeoutPriority, 
				_timeoutOnDeletion
			]
		];

		private _continue = true;
		private _previousReturn = nil;

		if _immediate then {
			private _args = _arguments;
			private _totalDelta = 0;
			private _localId = [_type, _event, _handler, clientOwner];
			private _eventName = _handler;

			if ((_conditionFunction isEqualTo {}) || (_conditionFunction isEqualTo "")) then {						
				if _iterationCount then {
					_previousReturn = _arguments call _function;
					["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

					if (_timeout == 1) then {
						_continue = false;
					};
				}
				else {
					_previousReturn = _arguments call _function;
				};
			}
			else {
				if !(_conditionFunction isEqualType {}) then {
					_conditionFunction = [[_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
				};
				
				if _iterationCount then {
					if _countConditionFailure then {
						if _timeoutOnConditionFailure then {
							if (_conditionArguments call _conditionFunction) then {
								_previousReturn = _arguments call _function;
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
							};
						}
						else {
							if (_conditionArguments call _conditionFunction) then {
								_previousReturn = _arguments call _function;
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;
							};
						};

						if (_timeout == 1) then {
							_continue = false;
						};
					}
					else {
						if _timeoutOnConditionFailure then {
							if (_conditionArguments call _conditionFunction) then {
								_previousReturn = _arguments call _function;
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

								if (_timeout == 1) then {
									_continue = false;
								};
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
							};
						}
						else {
							if (_conditionArguments call _conditionFunction) then {
								_previousReturn = _arguments call _function;
								["KH_eve_temporalExecutionStackHandler", [_handler, false, false]] call CBA_fnc_localEvent;

								if (_timeout == 1) then {
									_continue = false;
								};
							};
						};
					};
				}
				else {
					if _timeoutOnConditionFailure then {
						if (_conditionArguments call _conditionFunction) then {
							_previousReturn = _arguments call _function;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
						};
					}
					else {
						if (_conditionArguments call _conditionFunction) then {
							_previousReturn = _arguments call _function;
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
								"call ", _function, ";
								['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;"
							] joinString "");
						}
						else {
							_function;
						};
					}
					else {
						if !(_conditionFunction isEqualType {}) then {
							_conditionFunction = [[_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
						};
						
						if _iterationCount then {
							if _countConditionFailure then {
								if _timeoutOnConditionFailure then {
									compile ([
										"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
											call ", _function, ";
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										}
										else {
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								}
								else {
									compile ([
										"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
											call ", _function, ";
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
										"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
											call ", _function, ";
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										}
										else {
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								}
								else {
									compile ([
										"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
											call ", _function, ";
											['KH_eve_temporalExecutionStackHandler', ['", _handler, "', false, false]] call CBA_fnc_localEvent;
										};"
									] joinString "");
								};
							};
						}
						else {
							if _timeoutOnConditionFailure then {
								compile ([
									"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
										call ", _function, ";
									}
									else {
										['KH_eve_temporalExecutionStackHandler', ['", _handler, "', true, true]] call CBA_fnc_localEvent;
									};"
								] joinString "");
							}
							else {
								compile ([
									"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
										call ", _function, ";
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
					if _simpleDelta then {
						-1;
					}
					else {
						systemTime joinString "";
					},
					[_type, _event, _handler, clientOwner],
					_handler,
					_previousReturn
				]
			]
		];

		if (!_iterationCount && (_timeout != 0)) then {
			[
				[
					"TEMPORAL",
					false,
					[],
					{},
					[[1, false], !_timeoutPriority, false, false],
					[],
					{},
					true
				],
				_timeout,
				[_handler],
				{
					_args params ["_handler"];
					["KH_eve_temporalExecutionStackHandler", [_handler, true, true]] call CBA_fnc_localEvent;
				}
			] call KH_fnc_addEventHandler;
		};
	};

	case "DRAW_UI": {
		private _conditionArguments = _type param [1, []];
		private _conditionFunction = _type param [2, {}, ["", {}]];
		private _timeoutRules = _type param [3, [0, false, false], [[]]];
		private _timeoutArguments = _type param [4, []];
		private _timeoutFunction = _type param [5, {}, ["", {}]];
		private _handlerConditionArguments = call KH_fnc_generateUid;

		if !(_timeoutFunction isEqualType {}) then {
			_timeoutFunction = [[_timeoutFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
		};

		missionNamespace setVariable [_handlerConditionArguments, _conditionArguments];
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
					_function;
				}
				else {
					if !(_conditionFunction isEqualType {}) then {
						_conditionFunction = [[_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
					};
					
					if _timeoutOnConditionFailure then {
						compile ([
							"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
								call ", _function, ";
							}
							else {
								['KH_eve_drawUiExecutionStackHandler', ['", _handler, "', true]] call CBA_fnc_localEvent;
							};"
						] joinString "");
					}
					else {
						compile ([
							"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
								call ", _function, ";
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
				if (_timeoutFunction isNotEqualTo {}) then {
					compile ([
						"private _args = missionNamespace getVariable ['", _handlerArguments, "', []];
						private _eventName = missionNamespace getVariable ['", _eventName, "', ''];
						private _localId = missionNamespace getVariable ['", _handlerId, "', []];
						call ", _timeoutFunction, ";"
					] joinString "")
				}
				else {
					{};
				}, 
				_timeoutOnDeletion
			]
		];

		if (_timeout != 0) then {
			[
				[
					"TEMPORAL",
					false,
					[],
					{},
					[[1, false], false, false, false],
					[],
					{},
					true
				],
				_timeout,
				[_handler],
				{
					_args params ["_handler"];
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
				compile ([
					"private _handlerStack = (missionNamespace getVariable 'KH_var_inGameUiEventHandlerStack') get '", _event, "';
					private _handlerStackDeletions = missionNamespace getVariable 'KH_var_inGameUiEventHandlerStackDeletions';
					
					if (_handlerStackDeletions isNotEqualTo []) then {
						{
							if ((_x select 0) in _handlerStackDeletions) then {
								_handlerStackDeletions set [_handlerStack find (_x select 0), _forEachIndex];
							};
						} forEach _handlerStack;

						{
							if (_x isEqualType 0) then {
								_handlerStack deleteAt _x;
							};
						} forEach _handlerStackDeletions;

						_handlerStackDeletions = [];
					};

					if (_handlerStack isEqualTo []) exitWith {
						inGameUISetEventHandler [_event, ""];
						_handlerStack deleteAt ", _event, ";
					};

					{
						if !((_x select 0) in _handlerStackDeletions) then {
							call (_x select 1);
						};
					} forEach _handlerStack;"
				] joinString "")
			];
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

						{
							if (_x isEqualType 0) then {
								_handlerStack deleteAt _x;
							};
						} forEach _handlerStackDeletions;

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
	missionNamespace setVariable [_handlerId, [_handler, _persistentEntityId]];

	[
		[_handlerId, _handler, _persistentEntityId],
		{
			params ["_handlerId", "_handler", "_persistentEntityId"];
			missionNamespace setVariable [_handlerId, [_handler, _persistentEntityId], true];
		},
		"SERVER",
		true
	] call KH_fnc_execute;
	
	[_handler, _persistentEntityId];
};