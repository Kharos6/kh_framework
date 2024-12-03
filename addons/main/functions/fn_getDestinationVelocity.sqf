params ["_entity", "_position", "_time", ["_vector", false]];
private _direction = (getPosATL _position) vectorDiff (getPosATL _entity);
private _velocity = [(_direction select 0) / _time, (_direction select 1) / _time, (_direction select 2) / _time];

if _vector then {
	_velocity;
} 
else {
	_velocity distance [0, 0, 0];
};