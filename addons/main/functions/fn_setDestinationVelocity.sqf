params ["_entity", "_position", "_travelTime"];
private _destination = [];

if (_position isEqualType objNull) then {
	if !(isNull _position) then {
		_destination = getPosATL _position;
	};
}
else {
	_destination = _position;
};

private _direction = _destination vectorDiff (getPosATL _entity);
private _velocity = [(_direction select 0) / _travelTime, (_direction select 1) / _travelTime, ((_direction select 2) / _travelTime) + (0.5 * 9.807 * _travelTime)];

[
	[_entity, _velocity],
	{
		params ["_entity", "_velocity"];
		_entity setVelocity _velocity;
	},
	_entity,
	"THIS_FRAME"
] call KH_fnc_execute;

_velocity;