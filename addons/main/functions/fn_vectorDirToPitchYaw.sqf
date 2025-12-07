params [["_vectorDir", [0, 1, 0], [[], objNull]]];

if (_vectorDir isEqualType objNull) then {
    _vectorDir = vectorDir _vectorDir;
};

_vectorDir params [["_axisX", 0, [0]], ["_axisY", 1, [0]], ["_axisZ", 0, [0]]];
private _yaw = _axisX atan2 _axisY;

if (_yaw < 0) then {
    _yaw = _yaw + 360;
};

[_yaw, asin ((_axisZ min 1) max -1)];