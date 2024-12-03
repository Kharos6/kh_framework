params ["_entity", ["_relative", objNull]]; 
private _dir = vectorDir _entity; 
private _up = vectorUp _entity; 
 
if !(isNull _relative) then {  
	private _currentPosition = getPosATL _entity; 
	private _relativePosition = getPosATL _relative; 
	private _direction = _relativePosition vectorDiff _currentPosition; 
	private _distanceHorizontal = sqrt (((_direction select 0) ^ 2) + ((_direction select 1) ^ 2)); 
	private _distanceVertical = _direction select 2; 
	private _yaw = (_direction select 1) atan2 (_direction select 0); 
	private _pitch = _distanceVertical atan2 _distanceHorizontal; 
	_dir = [(cos _pitch) * (cos _yaw), (cos _pitch) * (sin _yaw), sin _pitch]; 
	_up = [0, 0, 1]; 
};

private _dirX = _dir select 0; 
private _dirY = _dir select 1; 
private _dirZ = _dir select 2; 
private _upX = _up select 0; 
private _upY = _up select 1; 
private _upZ = _up select 2; 
private _aroundX = ((-_dirZ) atan2 (sqrt (_dirX^2 + _dirY^2)) + 360) % 360; 
private _aroundY = ((360 - (_upX atan2 _upZ)) % 360); 
private _aroundZ = ((_dirX atan2 _dirY) + 360) % 360; 
[_aroundX, _aroundY, _aroundZ];