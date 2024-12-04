params ["_type", "_event", "_arguments", "_function"];
private _handlerArguments = [missionNamespace, "KH_var_eventHandlerArguments", _arguments, false] call KH_fnc_atomicVariable;
private _persistentId = "";
private _handlerId = -1;

if (_function isEqualType "") then {
	_function = missionNamespace getVariable [_function, {}];
};

private _expression = [
	"private _args = (missionNamespace getVariable ['", _handlerArguments, "', []]);
	call ", _function, ";"
] joinString "";

private _eventType = _type select 0;

switch true do {
	case (_eventType == "STANDARD"): {
		if !(_type select 2) then {
			_handlerId = (_type select 1) addEventHandler [_event, compile _expression];
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

			_handlerId = [
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
					-1, 
					[_entity, _persistentId], 
					{
						params ["_entity", "_persistentId"];
						_entity setVariable [_persistentId, false];
					}, 
					true
				],
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
	};

	case (_eventType == "MULTIPLAYER"): {
		_handlerId = (_type select 1) addMPEventHandler [_event, compile _expression];
	};

	case (_eventType == "CONTROL"): {
		_handlerId = (_type select 1) ctrlAddEventHandler [_event, compile _expression];
	};

	case (_eventType == "DISPLAY"): {
		_handlerId = (_type select 1) displayAddEventHandler [_event, compile _expression];
	};

	case (_eventType == "PUBLIC_VARIABLE"): {
		_handlerId = [missionNamespace, "KH_var_publicVariableEventHandler", true, false] call KH_fnc_atomicVariable;

		(_type select 1) addPublicVariableEventHandler (compile ([
			"if (missionNamespace getVariable ['", _handlerId, "', true]) then {", 
				_expression, 
			"};"
		] joinString ""));
	};

	case (_eventType == "MISSION"): {
		_handlerId = addMissionEventHandler [_event, compile _expression];
	};

	case (_eventType == "USER_ACTION"): {
		_handlerId = addUserActionEventHandler [_event, compile _expression];
	};

	case (_eventType == "MUSIC"): {
		_handlerId = addMusicEventHandler [_event, compile _expression];
	};

	case (_eventType == "CBA"): {
		_handlerId = [_event, compile _expression] call CBA_fnc_addEventHandler;
	};
};

if (persistentId == "") then {
	[_type, _event, _handlerId, clientOwner];
}
else {
	[_handlerId, _persistentId];
};