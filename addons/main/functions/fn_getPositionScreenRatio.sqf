params [["_camera", objNull, [objNull]], ["_position", [], [[], objNull]]];

if (_position isEqualType objNull) then {
    _position = _position modelToWorld [0, 0, 0];
};

private _cameraPosition = if (isNull _camera) then {
    worldToScreen _position;
}
else {
    _camera worldToScreen _position;
};

if (_cameraPosition isEqualTo []) exitWith {
    _cameraPosition;
};

_cameraPosition params ["_positionX", "_positionY"];
_positionX = _positionX - 0.5;
_positionY = _positionY - 0.5;
private _radialDistance = 1 - ((sqrt ((_positionX * _positionX) + (_positionY * _positionY))) / (sqrt 0.5));
_positionX = 1 - (2 * (abs _positionX));
_positionY = 1 - (2 * (abs _positionY));
private _averageDistance = (_positionX + _positionY) / 2;
private _strictDistance = _positionX * _positionY;
[(_strictDistance min 1) max 0, (_averageDistance min 1) max 0, (_radialDistance min 1) max 0, [(_positionX min 1) max 0, (_positionY min 1) max 0]];