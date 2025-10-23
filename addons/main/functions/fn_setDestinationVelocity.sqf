params [["_entity", objNull, [objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_travelTime", 0.001, [0]]];
_travelTime = _travelTime max 0.001;

if (_position isEqualType objNull) then {
	_position = getPosASL _position;
};

private _direction = _position vectorDiff (getPosASL _entity);
private _velocity = [(_direction select 0) / _travelTime, (_direction select 1) / _travelTime, ((_direction select 2) / _travelTime) + (0.5 * 9.807 * _travelTime)];

[
	[_entity, _velocity],
	{
		params ["_entity", "_velocity"];
		_entity setVelocity _velocity;
	},
	_entity,
	true,
	false
] call KH_fnc_execute;

_velocity;