params ["_objects", ["_local", true], ["_initialization", {}]];
private _simpleObjects = [];

{
	private _position = getPosWorld _x;
	private _vectorDirAndUp = [vectorDir _x, vectorUp _x];
	private _currentObject = (getModelInfo _x) select 1;
	deleteVehicle _x;

	if _local then {
		[
			[_initialization, _position, _vectorDirAndUp, _currentObject], 
			{
				params ["_initialization", "_position", "_vectorDirAndUp", "_currentObject"];	
				private _simpleObject = createSimpleObject [_currentObject, _position, true];
				_simpleObject setVectorDirAndUp _vectorDirAndUp;
				[_simpleObject] call _initialization;
			},
			["JIP", "GLOBAL", true, false, false],
			"THIS_FRAME"
		] call KH_fnc_execute;
	}
	else {
		private _simpleObject = createSimpleObject [_currentObject, _position, false];
		_simpleObject setVectorDirAndUp _vectorDirAndUp;
		_simpleObjects pushBack _simpleObject;
		[_simpleObject] call _initialization;
	};
} forEach _objects;

_simpleObjects;