params ["_handler"];

if ((count _handler) > 2) then {
	_handler params ["_type", "_event", "_handlerId", "_owner"];
	
	[
		[_type, _event, _handlerId],
		{
			params ["_type", "_event", "_handlerId"];
			_type params ["_eventType", "_target"];

			switch _eventType do {
				case "STANDARD": {
					_target removeEventHandler [_event, _handlerId];
				};

				case "MULTIPLAYER": {
					_target removeMPEventHandler [_event, _handlerId];
				};

				case "CONTROL": {
					_target ctrlRemoveEventHandler [_event, _handlerId];
				};

				case "DISPLAY": {
					_target displayRemoveEventHandler [_event, _handlerId];
				};

				case "PUBLIC_VARIABLE": {
					missionNamespace setVariable [_handlerId, false];
				};

				case "CLASS": {
					missionNamespace setVariable [_handlerId, false];
				};

				case "MISSION": {
					removeMissionEventHandler [_event, _handlerId];
				};

				case "USER_ACTION": {
					removeUserActionEventHandler [_event, _handlerId];
				};

				case "MUSIC": {
					removeMusicEventHandler [_event, _handlerId];
				};

				case "BIS_SCRIPTED": {
					[_target, _event, _handlerId] call BIS_fnc_removeScriptedEventHandler;
				};

				case "CBA": {
					private _currentStack = missionNamespace getVariable [_event, []];
					private _currentId = _handlerId select 1;
					private _deletion = -1;

					{
						if ((_x select 1) == _currentId) then {
							_deletion = _forEachIndex;
							break;
						};
					} forEach _currentStack;

					if (_deletion != -1) then {
						_currentStack deleteAt _deletion;
						missionNamespace setVariable [_event, _currentStack];
					};
				};
			};
		},
		_owner,
		"THIS_FRAME"
	] call KH_fnc_execute;
}
else {
	[
		_handler,
		{
			params ["_handlerId", "_persistentEntityId"];
			missionNamespace setVariable [_persistentEntityId, false, true];
			[_handlerId, "TERMINATE"] call KH_fnc_manageHandler;
		},
		"SERVER",
		"THIS_FRAME"
	] call KH_fnc_execute;
};

true;