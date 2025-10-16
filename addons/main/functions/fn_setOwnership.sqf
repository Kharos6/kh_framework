params [["_entities", [], [[], objNull, teamMemberNull, grpNull]], ["_owner", 0, [0, objNull]], ["_recreate", false, [true]], ["_init", {}, [{}]]];

if !(_entites isEqualType []) then {
	_entities = [_entities];
};

if (_owner isEqualType objNull) then {
	_owner = owner _owner;
};

private _parsedUnits = [];

{
	if (_x isEqualType teamMemberNull) then {
		(agent _x) setOwner _owner;
		continue;
	};

	if (_x isEqualType grpNull) then {
		{
			_parsedUnits pushBackUnique _x;
		} forEach (units _x);

		continue;
	};

	if !(isNull (group _x)) then {
		_parsedUnits pushBackUnique _x;
	}
	else {
		_x setOwner _owner;
	};
} forEach _entities;

if (_parsedUnits isEqualTo []) exitWith {};
private _groups = [];

{
	private _group = group _x;
	_groups pushBackUnique _group;
	private _attributes = [_x] call KH_fnc_getUnitAttributes;
	
	if _recreate then {
		[
			[_owner, _x, _group, _init, _attributes],
			{
				(_this select [1]) params ["_unit", "_group", "_init", "_attributes"];
				private _newUnit = _group createUnit [_attributes select 0, _unit, [], 0, "CAN_COLLIDE"];
				deleteVehicle _unit;
				[_newUnit, _attributes, [], true] call KH_fnc_setUnitAttributes;
				[_newUnit] call _init;
			},
			_owner,
			[
				{
					params ["_owner", "_unit"];
					((owner _unit) isEqualTo _owner);
				},
				true,
				30,
				0,
				true,
				true
			],
			false
		] call KH_fnc_execute;
	}
	else {
		[
			[_owner, _x, _init, _attributes],
			{
				(_this select [1]) params ["_unit", "_init", "_attributes"];
				[_unit, _attributes, [], true] call KH_fnc_setUnitAttributes;
				[_unit] call _init;
			},
			_owner,
			[
				{
					params ["_owner", "_unit"];
					((owner _unit) isEqualTo _owner);
				},
				true,
				30,
				0,
				true,
				true
			],
			false
		] call KH_fnc_execute;
	};
} forEach _parsedUnits;

{
	_x setGroupOwner _owner;
} forEach _groups;

nil;