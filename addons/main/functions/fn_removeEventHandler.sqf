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

				case "PUBLIC_VARIABLE";
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

				case "TEMPORAL": {
					KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;
				};

				case "IN_GAME_UI": {
					private _currentStack = KH_var_uiEventHandlerStack get _event;
					private _currentId = _handlerId select 0;
					private "_deletion";

					{
						if ((_x select 0) isEqualTo _currentId) then {
							_deletion = _x;
							break;
						};
					} forEach _currentStack;

					if (!isNil "_deletion") then {
						_currentStack deleteAt (_currentStack find _deletion);
					};

					if (_currentStack isEqualTo []) then {
						inGameUISetEventHandler [_event, ""];
						KH_var_uiEventHandlerStack deleteAt _event;
					};
				};

				case "PLAYER": {
					private _currentStack = KH_var_cbaPlayerEventHandlerStack get _event;
					private _currentId = _handlerId select 0;
					private "_deletion";

					{
						if ((_x select 0) isEqualTo _currentId) then {
							_deletion = _x;
							break;
						};
					} forEach _currentStack;

					if (!isNil "_deletion") then {
						_currentStack deleteAt (_currentStack find _deletion);
					};
				};

				case "CBA": {
					private _currentStack = KH_var_cbaEventHandlerStack get _event;
					private _currentId = _handlerId select 0;
					private "_deletion";

					{
						if ((_x select 0) isEqualTo _currentId) then {
							_deletion = _x;
							break;
						};
					} forEach _currentStack;

					if (!isNil "_deletion") then {
						_currentStack deleteAt (_currentStack find _deletion);
					};
				};
			};
		},
		_owner,
		"THIS_FRAME"
	] call KH_fnc_execute;
}
else {
	missionNamespace setVariable [_persistentEntityId, false];

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