params ["_array"];
private _number = 0;

{
	_number = _number + _x;
} forEach _array;

_number;