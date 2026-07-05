params [["_object", objNull, [objNull]], ["_local", true, [true]], ["_init", {}, [{}]]];
private _simpleObject = objNull;
private _position = getPosWorld _object;
private _vectorDirAndUp = [vectorDir _object, vectorUp _object];
private _currentObject = (getModelInfo _object) select 1;
deleteVehicle _object;

if _local then {
	execute [
		[_init, _position, _vectorDirAndUp, _currentObject], 
		{
			params ["_init", "_position", "_vectorDirAndUp", "_currentObject"];	
			private _simpleObject = createSimpleObject [_currentObject, _position, true];
			_simpleObject setVectorDirAndUp _vectorDirAndUp;
			[_simpleObject] call _init;
		},
		"GLOBAL",
		true,
		true
	];
}
else {
	_simpleObject = createSimpleObject [_currentObject, _position, false];
	_simpleObject setVectorDirAndUp _vectorDirAndUp;
	[_simpleObject] call _init;
};

_simpleObject;