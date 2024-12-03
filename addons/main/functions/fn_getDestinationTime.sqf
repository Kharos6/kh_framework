params ["_entity", "_position", ["_speed", 0]];
private _direction = _position vectorDiff (getPosATL _entity);
private _distance = _direction distance [0, 0, 0];

if (_speed == 0) then {
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