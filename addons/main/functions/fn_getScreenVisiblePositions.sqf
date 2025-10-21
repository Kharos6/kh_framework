params [["_camera", objNull, [objNull]], ["_stepX", 0.01, [0]], ["_stepY", 0.01, [0]], ["_shiftX", 0, [0]], ["_shiftY", 0, [0]], ["_raycast", false, [true]], ["_draw", [], [[]]]];
private _allPositions = [];
private _safeZoneX = safeZoneX + _shiftX;
private _safeZoneY = safeZoneY + _shiftY;

if (isNull _camera) then {
    if _raycast then {
        for "_positionX" from _safeZoneX to (1 + (abs _safeZoneX)) step _stepX do {
            for "_positionY" from _safeZoneY to (1 + (abs _safeZoneY)) step _stepY do {
                private _cameraPosition = AGLToASL (positionCameraToWorld [0, 0, 0]);

                _allPositions pushBack [
                    _cameraPosition, 
                    _cameraPosition vectorAdd ((screenToWorldDirection [_positionX, _positionY]) vectorMultiply viewDistance), 
                    [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit),
                    true, 
                    1, 
                    "VIEW",
                    "FIRE", 
                    true,
                    _draw
                ];
            };
        };
    }
    else {
        for "_positionX" from _safeZoneX to (1 + (abs _safeZoneX)) step _stepX do {
            for "_positionY" from _safeZoneY to (1 + (abs _safeZoneY)) step _stepY do {
                private _cameraPosition = positionCameraToWorld [0, 0, 0];
                private _position = screenToWorld [_positionX, _positionY];
                private _currentVectorDirection = _cameraPosition vectorFromTo _position;
                private _screenToWorldDirection = screenToWorldDirection [_positionX, _positionY];
                
                if ([(_currentVectorDirection select 0) toFixed 0, (_currentVectorDirection select 1) toFixed 0, (_currentVectorDirection select 2) toFixed 0] 
                    isNotEqualTo 
                    [(_screenToWorldDirection select 0) toFixed 0, (_screenToWorldDirection select 1) toFixed 0, (_screenToWorldDirection select 2) toFixed 0]
                   ) then {
                    _allPositions pushBack (_cameraPosition vectorAdd (_screenToWorldDirection vectorMultiply viewDistance));
                }
                else {
                    _allPositions pushBack _position;
                };
            };
        };
    };
}
else {
    if _raycast then {
        for "_positionX" from _safeZoneX to (1 + (abs _safeZoneX)) step _stepX do {
            for "_positionY" from _safeZoneY to (1 + (abs _safeZoneY)) step _stepY do {
                private _cameraPosition = getPosASL _camera;

                _allPositions pushBack [
                    _cameraPosition, 
                    _cameraPosition vectorAdd ((_camera screenToWorldDirection [_positionX, _positionY]) vectorMultiply viewDistance), 
                    [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit),
                    true, 
                    1, 
                    "VIEW",
                    "FIRE", 
                    true,
                    _draw
                ];
            };
        };
    }
    else {
        for "_positionX" from _safeZoneX to (1 + (abs _safeZoneX)) step _stepX do {
            for "_positionY" from _safeZoneY to (1 + (abs _safeZoneY)) step _stepY do {
                private _cameraPosition = _camera modelToWorld [0, 0, 0];
                private _position = _camera screenToWorld [_positionX, _positionY];
                private _currentVectorDirection = _cameraPosition vectorFromTo _position;
                private _screenToWorldDirection = _camera screenToWorldDirection [_positionX, _positionY];
                
                if ([(_currentVectorDirection select 0) toFixed 0, (_currentVectorDirection select 1) toFixed 0, (_currentVectorDirection select 2) toFixed 0] 
                    isNotEqualTo 
                    [(_screenToWorldDirection select 0) toFixed 0, (_screenToWorldDirection select 1) toFixed 0, (_screenToWorldDirection select 2) toFixed 0]
                   ) then {
                    _allPositions pushBack (_cameraPosition vectorAdd (_screenToWorldDirection vectorMultiply viewDistance));
                }
                else {
                    _allPositions pushBack _position;
                };
            };
        };
    };
};

if _raycast then {
    [_allPositions] call KH_fnc_raycast;
}
else {
    if (_draw isNotEqualTo []) then {
        _draw params [["_type", "", [""]], ["_arguments", [], [[]]], ["_duration", 0, [0, ""]]];

        if (_duration isEqualTo 0) then {
            _duration = true;
        };

        switch _type do {
            case "LINE": {
                _arguments params [["_color", [1, 1, 1, 1], [[]]], ["_width", 1, [0]]];

                {
                    [
                        _type, 
                        [
                            if (isNull _camera) then {
                                positionCameraToWorld [0, 0, 0];
                            }
                            else {
                                _camera modelToWorld [0, 0, 0];
                            }, 
                            _x, 
                            _color, 
                            _width
                        ], 
                        _duration, 
                        [], 
                        {}
                    ] call KH_fnc_draw3d;
                } forEach _allPositions;
            };

            case "LASER": {
                _arguments params [["_beamColor", [1, 1, 1], [[]]], ["_dotColor", [1, 1, 1], [[]]], ["_dotSize", 1, [0]], ["_beamThickness", 1, [0]], ["_beamMaxLength", 0, [0]], ["_ir", false, [true]]];

                {
                    [
                        _type, 
                        [
                            if (isNull _camera) then {
                                AGLToASL (positionCameraToWorld [0, 0, 0]);
                            }
                            else {
                                AGLToASL (_camera modelToWorld [0, 0, 0]);
                            }, 
                            AGLToASL _x, 
                            _beamColor, 
                            _dotColor, 
                            _dotSize, 
                            _beamThickness, 
                            _beamMaxLength, 
                            _ir
                        ], 
                        _duration, 
                        [], 
                        {}
                    ] call KH_fnc_draw3d;
                } forEach _allPositions;
            };

            case "ICON": {
                _arguments params [
                    ["_texture", "\a3\ui_f\data\IGUI\Cfg\Radar\radar_ca.paa", [""]],
                    ["_color", [1, 1, 1, 1], [[]]],
                    ["_width", 1, [0]],
                    ["_height", 1, [0]],
                    ["_angle", 1, [0]],
                    ["_text", "", [""]],
                    ["_shadow", false, [true, 0]],
                    ["_textSize", 1, [0]],
                    ["_font", "RobotoCondensedBold", [""]],
                    ["_textAlign", "center", [""]],
                    ["_drawSideArrows", false, [true]],
                    ["_offsetX", 0, [0]],
                    ["_offsetY", 0, [0]]
                ];

                {
                    [
                        _type, 
                        [_texture, _color, _x, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY], 
                        _duration,
                        [], 
                        {}
                    ] call KH_fnc_draw3d;
                } forEach _allPositions;
            };
        };
    };

    _allPositions;
};