params ["_units", "_loadouts"];

{
	_x setUnitLoadout (selectRandom _loadouts);
} forEach _units;