params ["_group", "_attributes", ["_id", true], ["_units", true], ["_waypoints", true]];
		
if _id then {
	_group setGroupIdGlobal (_attributes select 0);
};

if _units then {
	(_attributes select 1) joinSilent _group;
};

if _waypoints then {
	private _waypointArray = [];
	
	{
		_waypointArray pushBack [str (_x select 0), _x select 1];
	} forEach (_attributes select 3);
	
	private _waypointHash = createHashMapFromArray _waypointArray;
	
	[
		[_waypointHash],
		{
			params ["_waypointHash"];

			{
				private _waypoint = _group addWaypoint [_y select 11, 1, -1, _y select 10];
				_waypoint setWaypointBehaviour (_y select 0);
				_waypoint setWaypointCombatMode (_y select 1);
				_waypoint setWaypointCompletionRadius (_y select 2);
				_waypoint setWaypointDescription (_y select 3);
				_waypoint setWaypointForceBehaviour (_y select 4);
				_waypoint setWaypointFormation (_y select 5);
				_waypoint setWaypointHousePosition (_y select 6);
				_waypoint setWaypointLoiterAltitude (_y select 7);
				_waypoint setWaypointLoiterRadius (_y select 8);
				_waypoint setWaypointLoiterType (_y select 9);
				_waypoint setWaypointScript (_y select 12);
				_waypoint setWaypointSpeed (_y select 13);
				_waypoint setWaypointStatements (_y select 14);
				_waypoint setWaypointTimeout (_y select 15);
				_waypoint setWaypointType (_y select 16);
				_waypoint setWaypointVisible (_y select 17);
			} forEach _waypointHash;
		},
		"SERVER",
		true
	] call KH_fnc_execute;
};

true;