params [["_group", grpNull, [grpNull]], ["_attributes", [], [[]]], ["_overrideAttributes", [], [[]]]];
private _savedAttributes = _attributes param [4];

if (_overrideAttributes isNotEqualTo []) then {
	private _currentAttributes = ([_group] call KH_fnc_getGroupAttributes) select 3;

	{
		if (_x isEqualType 0) then {
			_savedAttributes set [_x, _currentAttributes select _x];
		}
		else {
			_x params [["_index", 0, [0]], "_value"];
			_savedAttributes set [_index, _value];
		};
	} forEach _overrideAttributes;
};

_savedAttributes params [
	"_leader",
	"_groupId",
	"_deletedWhenEmpty",
	"_formation",
	"_behaviour",
	"_combatMode",
	"_speedMode",
	"_waypoints",
	"_variables"
];

_group setGroupIdGlobal _groupId;

[
	[
		_group,
		_leader,
		_deletedWhenEmpty,
		_formation,
		_behaviour,
		_combatMode,
		_speedMode,
		_waypoints,
		_variables
	],
	{
		params [
			"_group",
			"_leader",
			"_deletedWhenEmpty",
			"_formation",
			"_behaviour",
			"_combatMode",
			"_speedMode",
			"_waypoints",
			"_variables"
		];

		_leader = ["OBJECT", _leader] call KH_fnc_getEntityByIdentifier;

		if !(isNull _leader) then {
			_group selectLeader _leader;
		};

		_group deleteGroupWhenEmpty _deletedWhenEmpty;
		_group setFormation _formation;
		_group setBehaviourStrong _behaviour;
		_group setCombatMode _combatMode;
		_group setSpeedMode _speedMode;

		{
			private _waypoint = _group addWaypoint [_x select 12, 1, -1, _x select 11];
			private _waypointAttachedVehicle = ["OBJECT", _x select 0] call KH_fnc_getEntityByIdentifier;

			if !(isNull _waypointAttachedVehicle) then {
				_waypoint waypointAttachVehicle _waypointAttachedVehicle;
			};

			_waypoint setWaypointBehaviour (_x select 1);
			_waypoint setWaypointCombatMode (_x select 2);
			_waypoint setWaypointCompletionRadius (_x select 3);
			_waypoint setWaypointDescription (_x select 4);
			_waypoint setWaypointForceBehaviour (_x select 5);
			_waypoint setWaypointFormation (_x select 6);
			_waypoint setWaypointHousePosition (_x select 7);
			_waypoint setWaypointLoiterAltitude (_x select 8);
			_waypoint setWaypointLoiterRadius (_x select 9);
			_waypoint setWaypointLoiterType (_x select 10);
			_waypoint setWaypointScript (_x select 13);
			_waypoint setWaypointSpeed (_x select 14);
			_waypoint setWaypointStatements (_x select 15);
			_waypoint setWaypointTimeout (_x select 16);
			_waypoint setWaypointType (_x select 17);
			_waypoint setWaypointVisible (_x select 18);
		} forEach _waypoints;

		{
			_group setVariable _x;
		} forEach _variables;
	},
	_group,
	true,
	false
] call KH_fnc_execute;

nil;