params [["_handler", [], [[]]]];

if ((count _handler) <= 2) exitWith {
	_handler params [["_handlerId", [], [[]]], ["_persistentExecutionId", "", [""]]];
	missionNamespace setVariable [_persistentExecutionId, false, true];
	[_handlerId] call KH_fnc_removeHandler;
	["KH_eve_eventHandlerRemoved", [_handler]] call CBA_fnc_globalEvent;
};

_handler params [["_type", [], [[]]], ["_event", "", [0, ""]], ["_handlerId", 0, [0, "", []]], ["_owner", 2, [0]]];

[
	[_type, _event, _handlerId],
	{
		params ["_type", "_event", "_handlerId"];
		_type params [["_eventType", "", [""]], ["_target", objNull, [true, objNull, grpNull, missionNamespace, locationNull, controlNull, displayNull]]];

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
				["KH_var_temporalExecutionStackMonitor", [_handlerId, true, false]] call CBA_fnc_localEvent;
			};

			case "DRAW_UI": {
				["KH_var_drawUiExecutionStackMonitor", [_handlerId, false]] call CBA_fnc_localEvent;
			};

			case "IN_GAME_UI": {
				KH_var_inGameUiEventHandlerStackDeletions pushBackUnique (_handlerId select 0);
			};

			case "PLAYER": {
				KH_var_playerEventHandlerStackDeletions pushBackUnique (_handlerId select 0);
			};

			case "CBA": {
				KH_var_cbaEventHandlerStackDeletions pushBackUnique (_handlerId select 0);
			};
		};

		["KH_eve_eventHandlerRemoved", [_handler]] call CBA_fnc_localEvent;
	},
	_owner,
	true,
	false
] call KH_fnc_execute;