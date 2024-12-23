params ["_entities", "_owner", ["_recreate", false], ["_initialization", {}]];
private _ownerId = 2;

if (_owner isEqualType 0) then {
	_ownerId = _owner;
}
else {
	_ownerId = owner _owner;
};

private _unitAttributes = createHashMap;

{
	private _group = group _x;
	
	if !(isNull _group) then {
		_unitAttributes insert [[hashValue _x, [_x] call KH_fnc_getUnitAttributes]];
	};
} forEach _entities;

{
	private _group = group _x;
	
	if !(isNull _group) then {
		private _attributes = _unitAttributes get (hashValue _x);
		
		if ((groupOwner _group) != _ownerId) then {
			_group setGroupOwner _ownerId;
		};
		
		if _recreate then {
			[
				[_x, _initialization, _attributes], 
				{
					params ["_unit", "_initialization", "_attributes"];
					private _previousUnit = _unit;
					_unit = (_attributes select 1) createUnit [_attributes select 0, _previousUnit, [], 0, "CAN_COLLIDE"];
					[_unit] joinSilent (_attributes select 1);
					deleteVehicle _previousUnit;
					[_unit, _attributes, false, true, true, true, true, true, true, true, true, true, true] call KH_fnc_setUnitAttributes;
					[_unit] call _initialization;
				},
				_ownerId,
				"THIS_FRAME"
			] call KH_fnc_execute;
		}
		else {
			if ((owner _x) != _ownerId) then {
				[
					{
						params ["_unit", "_ownerId"];
						((owner _unit) == _ownerId);
					},
					{
						params ["_unit", "_ownerId", "_attributes"];
						[_unit, _attributes, false, false, true, true, true, true, false, true, true, true, true] call KH_fnc_setUnitAttributes;
						[[_unit], _initialization, _ownerId, "THIS_FRAME"] call KH_fnc_execute;
					}, 
					[_x, _ownerId, _attributes], 
					30
				] call CBA_fnc_waitUntilAndExecute;
			}
			else {
				[_x, _attributes, false, false, true, true, true, true, false, true, true, true, true] call KH_fnc_setUnitAttributes;
				[[_x], _initialization, _ownerId, "THIS_FRAME"] call KH_fnc_execute;
			};
		};
	}
	else {
		_x setOwner _ownerId;
	};
} forEach _entities;

true;