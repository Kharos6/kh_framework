params ["_group"];
private _id = groupId _group;
private _units = [];

{
	if ((group _x) == _group) then {
		_units pushBack _x;
	};
} forEach [["REGULAR", "PLAYABLE"], false, false] call KH_fnc_getUnits;

private _waypoints = waypoints _group;
private _waypointAttributes = [];

{
	_waypointAttributes pushBack [
		_x, 
		[
			waypointBehaviour _x, waypointCombatMode _x, waypointCompletionRadius _x, waypointDescription _x, waypointForceBehaviour _x, waypointFormation _x, 
			waypointHousePosition _x, waypointLoiterAltitude _x, waypointLoiterRadius _x, waypointLoiterType _x, waypointName _x, waypointPosition _x, 
			waypointScript _x, waypointSpeed _x, waypointStatements _x, waypointTimeout _x, waypointType _x, waypointVisible _x
		]
	];
} forEach _waypoints;

[_id, _units, _waypoints, _waypointAttributes];