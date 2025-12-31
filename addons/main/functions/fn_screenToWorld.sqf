params [["_camera", objNull, [objNull]], ["_positionX", 0, [0]], ["_positionY", 0, [0]]];

if (isNull _camera) then {
    private _cameraPosition = positionCameraToWorld [0, 0, 0];
    private _position = screenToWorld [_positionX, _positionY];
    private _currentVectorDirection = _cameraPosition vectorFromTo _position;
    private _screenToWorldDirection = screenToWorldDirection [_positionX, _positionY];
    
    if ([(_currentVectorDirection select 0) toFixed 0, (_currentVectorDirection select 1) toFixed 0, (_currentVectorDirection select 2) toFixed 0] 
        isNotEqualTo 
        [(_screenToWorldDirection select 0) toFixed 0, (_screenToWorldDirection select 1) toFixed 0, (_screenToWorldDirection select 2) toFixed 0]
       ) then {
        _cameraPosition vectorAdd (_screenToWorldDirection vectorMultiply viewDistance);
    }
    else {
        _position;
    };
}
else {
    private _cameraPosition = _camera modelToWorld [0, 0, 0];
    private _position = _camera screenToWorld [_positionX, _positionY];
    private _currentVectorDirection = _cameraPosition vectorFromTo _position;
    private _screenToWorldDirection = _camera screenToWorldDirection [_positionX, _positionY];
    
    if ([(_currentVectorDirection select 0) toFixed 0, (_currentVectorDirection select 1) toFixed 0, (_currentVectorDirection select 2) toFixed 0] 
        isNotEqualTo 
        [(_screenToWorldDirection select 0) toFixed 0, (_screenToWorldDirection select 1) toFixed 0, (_screenToWorldDirection select 2) toFixed 0]
        ) then {
        _cameraPosition vectorAdd (_screenToWorldDirection vectorMultiply viewDistance);
    }
    else {
        _position;
    };
};