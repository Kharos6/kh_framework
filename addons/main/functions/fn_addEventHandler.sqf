params [["_type", "", ["", []]], ["_event", "", ["", 0]], "_arguments", ["_function", {}, ["", {}]]];
private _handlerArguments = call KH_fnc_generateUid;
missionNamespace setVariable [_handlerArguments, _arguments];
private _handlerId = call KH_fnc_generateUid;
missionNamespace setVariable [_handlerId, -1];
private _eventName = call KH_fnc_generateUid;
missionNamespace setVariable [_eventName, _event];
private "_persistentEventId";
private "_persistentEntityId";
private _handler = -1;
_function = [_arguments, [_function] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

private _expression = compile ([
	"private _args = missionNamespace getVariable ['", _handlerArguments, "', []];
	private _eventName = missionNamespace getVariable ['", _eventName, "', ''];
	private _localId = missionNamespace getVariable ['", _handlerId, "', []];
	call ", _function, ";"
] joinString "");

private _eventType = if (_type isEqualType []) then {
	_type select 0;
}
else {
	_type;
};

switch _eventType do {
	case "STANDARD": {
		if !(_type select 2) then {
			_handler = (_type select 1) addEventHandler [_event, _expression];
		}
		else {
			_persistentEventId = call KH_fnc_generateUid;
			_persistentEntityId = call KH_fnc_generateUid;
			private _persistentEntity = call KH_fnc_generateUid;

			[
				[_persistentEntity],
				{
					params ["_persistentEntity"];
					missionNamespace setVariable [_persistentEntity, _this select 1, true];
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

			_handler = [
				[_type, _event, _arguments, _function, _persistentEventId, _persistentEntityId, _persistentEntity], 
				{
					params ["_type", "_event", "_arguments", "_function", "_persistentEventId", "_persistentEntityId", "_persistentEntity"];
					missionNamespace setVariable [_persistentEntityId, true];
					_type set [2, false];

					private _newFunction = compile ([
						"if (missionNamespace getVariable ['", _persistentEntityId, "', true]) then {
							call ", _function, ";
						}
						else {
							(missionNamespace getVariable ['", _persistentEntity, "', objNull]) removeEventHandler [_thisEvent, _thisEventHandler];
						};"
					] joinString "");

					private _eventId = [_type, _event, _arguments, _newFunction] call KH_fnc_addEventHandler;
					missionNamespace setVariable [_persistentEventId, _eventId];
				},
				[
					"PERSISTENT", 
					_type select 1, 
					[_persistentEventId, _persistentEntityId], 
					{
						params ["_persistentEventId", "_persistentEntityId"];
						private _eventId = missionNamespace getVariable [_persistentEventId, []];
						missionNamespace setVariable [_persistentEntityId, false];

						if (_eventId isNotEqualTo []) then {
							[_eventId] call KH_fnc_removeEventHandler;
						};
					}, 
					true
				],
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
	};

	case "MULTIPLAYER": {
		_handler = (_type select 1) addMPEventHandler [_event, _expression];
	};

	case "CONTROL": {
		_handler = (_type select 1) ctrlAddEventHandler [_event, _expression];
	};

	case "DISPLAY": {
		_handler = (_type select 1) displayAddEventHandler [_event, _expression];
	};

	case "CLASS": {
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
					"{
						call (_x select 1);
					} forEach ((missionNamespace getVariable 'KH_var_cbaPlayerEventHandlerStack') get '", _event, "');"
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
			"if (missionNamespace getVariable ['", _handler, "', true]) then {
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

	case "TEMPORAL": {
		private _simpleDelta = _type param [1, true, [true]];
		private _conditionArguments = _type param [2, []];
		private _conditionFunction = _type param [3, {}, ["", {}]];
		private _timeout = _type param [4, 0, [0]];
		private _timeoutOnConditionFailure = _type param [5, false, [true]]; 
		private _timeoutArguments = _type param [6, []];
		private _timeoutFunction = _type param [7, {}, ["", {}]];
		_timeoutFunction = [_arguments, [_timeoutFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
		_handler = call KH_fnc_generateUid;
		private _handlerConditionArguments = call KH_fnc_generateUid;
		missionNamespace setVariable [_handlerConditionArguments, _conditionArguments];
		private _handlerTimeoutArguments = call KH_fnc_generateUid;
		missionNamespace setVariable [_handlerTimeoutArguments, _timeoutArguments];
		private _args = _arguments;
		private _handle = _handler;
		private _totalDelta = 0;	

		KH_var_temporalExecutionStack pushBack [
			[],
			if ((_conditionFunction isEqualTo {}) || (_conditionFunction isEqualTo "")) then {
				call _expression;
				_expression;
			}
			else {
				_conditionFunction = [_arguments, [_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

				if (_conditionArguments call _conditionFunction) then {
					call _expression;
				}
				else {
					if _timeoutOnConditionFailure then {
						_timeoutArguments call _timeoutFunction;
						KH_var_temporalExecutionStackDeletions pushBackUnique _handler;
					};
				};

				compile ([
					"if ((missionNamespace getVariable ['", _handlerConditionArguments, "', []]) call ", _conditionFunction, ") then {
						call ", _expression, ";
					}
					else {
						if ", _timeoutOnConditionFailure, " then {
							(missionNamespace getVariable 'KH_var_temporalExecutionStackDeletions') pushBackUnique '", _handler, "';
							(missionNamespace getVariable ['", _handlerTimeoutArguments, "', []]) call ", _timeoutFunction, "
						};
					};"
				] joinString "");
			},
			_event,
			if (_event >= 0) then {
				diag_tickTime;
			}
			else {
				diag_frameNo;
			},
			if _simpleDelta then {
				-1;
			}
			else {
				systemTime joinString "";
			},
			_handler
		];

		if (_timeout != 0) then {
			[
				{
					params ["_timeoutArguments", "_timeoutFunction", "_handler"];
					KH_var_temporalExecutionStackDeletions pushBackUnique _handler;
					_timeoutArguments call _timeoutFunction;
				}, 
				[_timeoutArguments, _timeoutFunction, _handler], 
				_timeout
			] call CBA_fnc_waitAndExecute;
		};
	};

	case "BIS_SCRIPTED": {
		_handler = [_type select 1, _event, _expression] call BIS_fnc_addScriptedEventHandler;
	};

	case "IN_GAME_UI": {
		private _currentStack = KH_var_inGameUiEventHandlerStack get _event;

		if (isNil "_currentStack") then {
			KH_var_inGameUiEventHandlerStack set [_event, []];
			_currentStack = KH_var_inGameUiEventHandlerStack get _event;

			inGameUISetEventHandler [
				_event,
				[
					"{
						call (_x select 1);
					} forEach ((missionNamespace getVariable 'KH_var_inGameUiEventHandlerStack') get '", _event, "');"
				] joinString ""
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
					"{
						call (_x select 1);
					} forEach ((missionNamespace getVariable 'KH_var_cbaEventHandlerStack') get '", _event, "');"
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
	[_handler, _persistentEntityId];
};