params [["_entities", [], [[], objNull, teamMemberNull, grpNull]], ["_owner", 0, [0, objNull]], ["_recreate", false, [true]], ["_init", {}, [{}]]];

if !(_entities isEqualType []) then {
	_entities = [_entities];
};

if (_owner isEqualType objNull) then {
	_owner = owner _owner;
};

private _unitLoadoutChecks = [];
private _parsedUnits = [];

{
	if (_x isEqualType teamMemberNull) then {
		private _agent = agent _x;
		_agent setOwner _owner;
		_unitLoadoutChecks pushBackUnique [_agent, getUnitLoadout _agent];
		continue;
	};

	if (_x isEqualType grpNull) then {
		{
			_parsedUnits pushBackUnique _x;
			_unitLoadoutChecks pushBackUnique [_x, getUnitLoadout _x];
		} forEach (units _x);

		continue;
	};

	if (_x isKindOf "Man") then {
		if !(isNull (group _x)) then {
			_parsedUnits pushBackUnique _x;
			_unitLoadoutChecks pushBackUnique [_x, getUnitLoadout _x];
		}
		else {
			_x setOwner _owner;
		};
	}
	else {
		if ((crew _x) isNotEqualTo []) then {
			{
				_parsedUnits pushBackUnique _x;
				_unitLoadoutChecks pushBackUnique [_x, getUnitLoadout _x];
			} forEach (crew _x);
		}
		else {
			_x setOwner _owner;
		};
	};
} forEach _entities;

if (_parsedUnits isEqualTo []) exitWith {};
private _groups = [];

{
	private _group = group _x;
	_groups pushBackUnique _group;
	private _attributes = [_x] call KH_fnc_getUnitAttributes;
	
	if _recreate then {
		private _edenId = _x getVariable ["KH_var_edenId", ""];

		execute [
			[_owner, _x, _group, _init, _attributes, _edenId],
			{
				params ["_owner", "_unit", "_group", "_init", "_attributes", "_edenId"];
				
				execute [
					[_unit, _group, _init, _attributes, _edenId],
					{
						params ["_unit", "_group", "_init", "_attributes", "_edenId"];
						private _newUnit = _group createUnit [_attributes select 3, _unit, [], 0, "CAN_COLLIDE"];

						if (_edenId isNotEqualTo "") then {
							_newUnit setVariable ["KH_var_edenId", _edenId, true];
							KH_var_edenIds set [_edenId, _newUnit];

							execute [
								[_newUnit, _edenId],
								{
									params ["_newUnit", "_edenId"];
									KH_var_edenIds set [_edenId, _newUnit];
								},
								"REMOTE",
								true,
								["KH_var_edenIdRecreation", _edenId, _newUnit] joinString "_"
							];
						};

						private _variableName = vehicleVarName _unit;
						deleteVehicle _unit;
						[_newUnit, _attributes, [], true] call KH_fnc_setUnitAttributes;
						[_newUnit] call _init;
					},
					_owner,
					true,
					false
				];

				[_handlerId] call KH_fnc_removeHandler;
			},
			true,
			[
				{
					params ["_owner", "_unit"];
					((owner _unit) isEqualTo _owner);
				},
				false,
				0,
				30,
				{},
				false,
				true
			],
			false
		];
	}
	else {
		execute [
			[_owner, _x, _init, _attributes],
			{
				params ["_owner", "_unit", "_init", "_attributes"];

				execute [
					[_unit, _init, _attributes],
					{
						params ["_unit", "_init", "_attributes"];
						[_unit, _attributes, [], true] call KH_fnc_setUnitAttributes;
						[_unit] call _init;
					},
					_owner,
					true,
					false
				];

				[_handlerId] call KH_fnc_removeHandler;
			},
			true,
			[
				{
					params ["_owner", "_unit"];
					((owner _unit) isEqualTo _owner);
				},
				false,
				0,
				30,
				{},
				false,
				true
			],
			false
		];
	};
} forEach _parsedUnits;

{
	_x setGroupOwner _owner;
} forEach _groups;

execute [
	[_unitLoadoutChecks],
	{
		params ["_unitLoadoutChecks"];

		{
			_x params ["_unit", "_loadout"];

			if ((getUnitLoadout _unit) isNotEqualTo _loadout) then {
				_unit setUnitLoadout _loadout;
			};
		} forEach _unitLoadoutChecks;
	},
	true,
	"1",
	false
];

nil;