params [["_entity", objNull, [objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_time", 0.001, [0]], ["_vector", false, [true]]];
_time = _time max 0.001;

if (_position isEqualType objNull) then {
	_position = getPosASL _position;
};

private _direction = _position vectorDiff (getPosASL _entity);
private _velocity = [(_direction select 0) / _time, (_direction select 1) / _time, (_direction select 2) / _time];

if _vector then {
	_velocity;
} 
else {
	_velocity distance [0, 0, 0];
};