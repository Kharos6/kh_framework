params [["_camera", objNull, [objNull]], ["_positionX", 0, [0]], ["_positionY", 0, [0]]];

if (isNull _camera) then {
    private _cameraPosition = positionCameraToWorld [0, 0, 0];
    private _position = screenToWorld [_positionX, _positionY];
    private _screenToWorldDirection = screenToWorldDirection [_positionX, _positionY];
    private _currentVectorDirection = (AGLToASL _cameraPosition) vectorFromTo (AGLToASL _position);
    
    if (((_currentVectorDirection vectorDotProduct _screenToWorldDirection) < 0.995) || (_position isEqualTo [0, 0, 0])) then {
        ASLToAGL ((AGLToASL _cameraPosition) vectorAdd (_screenToWorldDirection vectorMultiply viewDistance));
    }
    else {
        _position;
    };
}
else {
    private _cameraPosition = _camera modelToWorld [0, 0, 0];
    private _position = _camera screenToWorld [_positionX, _positionY];
    private _screenToWorldDirection = _camera screenToWorldDirection [_positionX, _positionY];
    private _currentVectorDirection = (AGLToASL _cameraPosition) vectorFromTo (AGLToASL _position);
    
    if (((_currentVectorDirection vectorDotProduct _screenToWorldDirection) < 0.995) || (_position isEqualTo [0, 0, 0])) then {
        ASLToAGL ((AGLToASL _cameraPosition) vectorAdd (_screenToWorldDirection vectorMultiply viewDistance));
    }
    else {
        _position;
    };
};