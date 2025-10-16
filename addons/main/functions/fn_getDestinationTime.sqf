params [["_entity", objNull, [objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_speed", 0.001, [0]]];
_speed = _speed max 0.001;

if (_position isEqualType objNull) then {
	_position = getPosATL _position;
};

private _direction = _position vectorDiff (getPosATL _entity);
private _distance = _direction distance [0, 0, 0];

if (_speed isEqualTo 0) then {
	private _currentVelocity = velocity _entity;

	if (_currentVelocity isNotEqualTo [0, 0, 0]) then {
		_speed = _currentVelocity distance [0, 0, 0];
		(_distance / _speed);
	}
	else {
		-1;
	};
}
else {
	(_distance / _speed);
};