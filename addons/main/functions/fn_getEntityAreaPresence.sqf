params ["_entities", "_area"];
private _entitiesPresent = [];
private _entitiesAbsent = [];

{
	if (_x inArea _area) then {
		_entitiesPresent pushBack _x;
	}
	else {
		_entitiesAbsent pushBack _x;
	};
} forEach _entities;

[_entitiesPresent, _entitiesAbsent];