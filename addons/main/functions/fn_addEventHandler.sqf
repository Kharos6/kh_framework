params ["_type", "_event", "_arguments", "_function"];
private _handlerArguments = [] call KH_fnc_generateUid;
missionNamespace setVariable [_handlerArguments, _arguments];
private _handlerId = [] call KH_fnc_generateUid;
missionNamespace setVariable [_handlerId, -1];
private _eventName = [] call KH_fnc_generateUid;
missionNamespace setVariable [_eventName, _event];
private _persistentId = "";
private _persistentEntityId = "";
private _handler = -1;
private _eventType = "";
_function = [_arguments, [_function] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

private _expression = compile ([
	"private _args = missionNamespace getVariable ['", _handlerArguments, "', []];
	private _eventName = missionNamespace getVariable ['", _eventName, "', ''];
	private _localId = missionNamespace getVariable ['", _handlerId, "', []];
	call ", _function, ";"
] joinString "");

if (_type isEqualType []) then {
	_eventType = _type select 0;
}
else {
	_eventType = _type;
};

switch _eventType do {
	case "STANDARD": {
		if !(_type select 2) then {
			_handler = (_type select 1) addEventHandler [_event, _expression];
		}
		else {
			_persistentId = [] call KH_fnc_generateUid;
			_persistentEntityId = [] call KH_fnc_generateUid;
			_persistentEntity = [] call KH_fnc_generateUid;

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
				[_type, _event, _arguments, _function, _persistentId, _persistentEntityId, _persistentEntity], 
				{
					params ["_type", "_event", "_arguments", "_function", "_persistentId", "_persistentEntityId", "_persistentEntity"];
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
					missionNamespace setVariable [_persistentId, _eventId];
				},
				[
					"PERSISTENT", 
					_type select 1, 
					[_persistentId, _persistentEntityId], 
					{
						params ["_persistentId", "_persistentEntityId"];
						private _eventId = missionNamespace getVariable [_persistentId, []];
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

	case "PUBLIC_VARIABLE": {
		_handler = [missionNamespace, "KH_var_publicVariableEventHandler", true, false] call KH_fnc_atomicVariable;

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

	case "CBA": {
		private _handlerStackId = format ["KH_var_cbaEventHandlerStackEvent%1", _event];

		if !(_handlerStackId in KH_var_cbaEventHandlerStack) then {
			KH_var_cbaEventHandlerStack pushBack _handlerStackId;

			[
				_event, 
				{
					{
						_x params ["_function"];
						call _function;
					} forEach (missionNamespace getVariable [format ["KH_var_cbaEventHandlerStackEvent%1", _eventName], []]);
				}
			] call CBA_fnc_addEventHandler;
		};

		_handler = [_expression, [] call KH_fnc_generateUid];
		private _currentStack = missionNamespace getVariable [_handlerStackId, []];
		_currentStack pushBack _handler;
		missionNamespace setVariable [_handlerStackId, _currentStack];
		_event = _handlerStackId;
	};
};

if (_persistentId == "") then {
	missionNamespace setVariable [_handlerId, [_type, _event, _handler, clientOwner]];
	[_type, _event, _handler, clientOwner];
}
else {
	missionNamespace setVariable [_handlerId, [_handler, _persistentEntityId]];
	[_handler, _persistentEntityId];
};