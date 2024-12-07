params ["_type", "_event", "_arguments", "_function"];
private _handlerArguments = [missionNamespace, "KH_var_eventHandlerArguments", _arguments, false] call KH_fnc_atomicVariable;
private _handlerId = [missionNamespace, "KH_var_eventHandlerId", -1, false] call KH_fnc_atomicVariable;
private _persistentId = "";
private _handler = -1;

if (_function isEqualType "") then {
	_function = missionNamespace getVariable [_function, {}];
};

private _expression = [
	"private _args = (missionNamespace getVariable ['", _handlerArguments, "', []]);
	private _localId = (missionNamespace getVariable ['", _handlerId, "', -1]);
	call ", _function, ";"
] joinString "";

private _eventType = _type select 0;

switch true do {
	case (_eventType == "STANDARD"): {
		if !(_type select 2) then {
			_handler = (_type select 1) addEventHandler [_event, compile _expression];
			missionNamespace setVariable [_handlerId, _handler];
		}
		else {
			_persistentId = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
			private _entity = _type select 1;

			[
				[_entity, _persistentId],
				{
					params ["_entity", "_persistentId"];
					_entity setVariable [_persistentId, true, true];
					missionNamespace setVariable [_persistentId, true, true];
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

			_handler = [
				[_type, _event, _arguments, _function, _persistentId], 
				{
					params ["_type", "_event", "_arguments", "_function", "_persistentId"];
					private _newType = [_type select 0, _type select 1, false];
					private _persistentEntityId = [missionNamespace, "KH_var_persistentEventHandlerEntity", _entity, false] call KH_fnc_atomicVariable;
					
					private _newFunction = compile ([
						"if (((missionNamespace getVariable '", _persistentEntityId, "') getVariable ['", _persistentId, "', true]) && (missionNamespace getVariable ['", _persistentId, "', true])) then {
							call ", _function, ";
						}
						else {
							removeEventHandler [_thisEvent, _thisEventHandler];
						};"
					] joinString "");

					[_newType, _event, _arguments, _newFunction] call KH_fnc_addEventHandler;
				},
				[
					"PERSISTENT", 
					_entity, 
					[_entity, _persistentId], 
					{
						params ["_entity", "_persistentId"];
						_entity setVariable [_persistentId, false];
					}, 
					true
				],
				"THIS_FRAME"
			] call KH_fnc_execute;

			missionNamespace setVariable [_handlerId, _handler];
		};
	};

	case (_eventType == "MULTIPLAYER"): {
		_handler = (_type select 1) addMPEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "CONTROL"): {
		_handler = (_type select 1) ctrlAddEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "DISPLAY"): {
		_handler = (_type select 1) displayAddEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "PUBLIC_VARIABLE"): {
		_handler = [missionNamespace, "KH_var_publicVariableEventHandler", true, false] call KH_fnc_atomicVariable;
		missionNamespace setVariable [_handlerId, _handler];

		(_type select 1) addPublicVariableEventHandler (compile ([
			"if (missionNamespace getVariable ['", _handler, "', true]) then {", 
				_expression, 
			"};"
		] joinString ""));
	};

	case (_eventType == "MISSION"): {
		_handler = addMissionEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "USER_ACTION"): {
		_handler = addUserActionEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "MUSIC"): {
		_handler = addMusicEventHandler [_event, compile _expression];
		missionNamespace setVariable [_handlerId, _handler];
	};

	case (_eventType == "CBA"): {
		_handler = [_event, compile _expression] call CBA_fnc_addEventHandler;
		missionNamespace setVariable [_handlerId, _handler];
	};
};

if (_persistentId == "") then {
	[_type, _event, _handler, clientOwner];
}
else {
	[_handler, _persistentId];
};