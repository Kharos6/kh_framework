params ["_entities", "_trigger"];
private _triggerList = list _trigger;
private _entitiesPresent = [];
private _entitiesAbsent = [];

{
	if (_x in _triggerList) then {
		_entitiesPresent pushBack _x;
	}
	else {
		_entitiesAbsent pushBack _x;
	};
} forEach _entities;

[_entitiesPresent, _entitiesAbsent];