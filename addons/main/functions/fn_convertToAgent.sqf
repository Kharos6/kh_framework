params ["_units", ["_init", {}]];
private _agents = [];

{
	private _attributes = [_x] call KH_fnc_getUnitAttributes;
	private _currentAgent = createAgent [_attributes select 0, _x, [], 0, "CAN_COLLIDE"];
	deleteVehicle _x;
	[_unit, _attributes, false, true, true, true, true, true, true, false, false, false, true] call KH_fnc_setUnitAttributes;
	_agents pushBack _currentAgent;
	[_currentAgent] call _init;
} forEach _units;

_agents;