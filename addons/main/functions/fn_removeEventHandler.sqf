params ["_handler"];

if ((count _handler) > 2) then {
	[
		[_handler select 0, _handler select 1, _handler select 2],
		{
			params ["_type", "_event", "_handlerId"];
			_type params ["_eventType", "_target"];

			switch true do {
				case (_eventType == "STANDARD"): {
					_target removeEventHandler [_event, _handlerId];
				};

				case (_eventType == "MULTIPLAYER"): {
					_target removeMPEventHandler [_event, _handlerId];
				};

				case (_eventType == "CONTROL"): {
					_target ctrlRemoveEventHandler [_event, _handlerId];
				};

				case (_eventType == "DISPLAY"): {
					_target displayRemoveEventHandler [_event, _handlerId];
				};

				case (_eventType == "PUBLIC_VARIABLE"): {
					missionNamespace setVariable [_handlerId, false];
				};

				case (_eventType == "MISSION"): {
					removeMissionEventHandler [_event, _handlerId];
				};

				case (_eventType == "USER_ACTION"): {
					removeUserActionEventHandler [_event, _handlerId];
				};

				case (_eventType == "MUSIC"): {
					removeMusicEventHandler [_event, _handlerId];
				};

				case (_eventType == "CBA"): {
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
		_handler select 3,
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