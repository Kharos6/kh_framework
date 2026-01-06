if ((count _this) isEqualTo 1) exitWith {
    params [["_visibilityArguments", [], [[]]]];
    private _result = false;

    {
        if (_x call KH_fnc_getPositionVisibility) then {
            _result = true;
            break;
        };
    } forEach _visibilityArguments;

    _result;
};

params [
    ["_start", [0, 0, 0], [[], objNull]], 
    ["_end", [0, 0, 0], [[], objNull]], 
    ["_direction", [0, 1, 0], [[], objNull]], 
    ["_fov", 360, [0, []]],
    ["_minimumDistance", 0, [0]], 
    ["_maximumDistance", 0, [0]],
    ["_raycast", false, [true, objNull]]
];

private ["_horizontalFov", "_verticalFov", "_minimalFov"];

if (_fov isEqualType []) then {
    _horizontalFov = abs (_fov select 0);
    _verticalFov = abs (_fov select 1);
    _minimalFov = (abs (_fov select 0)) min (abs (_fov select 1));
}
else {
    _horizontalFov = _fov;
    _verticalFov = _fov;
    _minimalFov = abs _fov;
};

private _ignored = if (_raycast isNotEqualTo false) then {
    if (_start isEqualType objNull) then {
        if (_end isEqualType objNull) then {
            [_start, objectParent _start, attachedTo _start] + (attachedObjects _start) + [_end, objectParent _end, attachedTo _end] + (attachedObjects _end);
        }
        else {
            [_start, objectParent _start, attachedTo _start] + (attachedObjects _start);
        };
    }
    else {
        if (_end isEqualType objNull) then {
            [_end, objectParent _end, attachedTo _end] + (attachedObjects _end);
        }
        else {
            [];
        };
    };
}
else {
    [];
};

if (_start isEqualType objNull) then {
    if (isPlayer _start) then {
        _verticalFov = (getObjectFOV _start) * (180 / pi) * _verticalFov;
        _horizontalFov = 2 * (atan (((_start getVariable ["KH_var_playerAspectRatio", 1.777777]) * _horizontalFov) * tan ((_verticalFov / 2) * (pi / 180)))) * (180 / pi);
        _maximumDistance = _start getVariable ["KH_var_playerViewDistance", _maximumDistance];
        _start = AGLToASL (_start getVariable ["KH_var_playerCameraPosition", eyePos _start]);
    }
    else {
        if (_start isKindOf "Man") then {
            _start = eyePos _start;
        }
        else {
            _start = AGLToASL (unitAimPositionVisual _start);
        };
    };
};

if (_end isEqualType objNull) then {
    _end = AGLToASL (unitAimPositionVisual _end);
};

if (_maximumDistance isEqualTo 0) then {
    _maximumDistance = 999999;
};

if ((_start vectorDistance _end) > _maximumDistance) exitWith {
    false;
};

if ((_start vectorDistance _end) < _minimumDistance) exitWith {
    true;
};

if (_minimalFov >= 360) exitWith {
    if (_raycast isEqualTo true) then {
        (
            [
                _start,
                _end,
                _ignored,
                true,
                1,
                "VIEW",
                "NONE",
                true,
                []
            ] call KH_fnc_raycast
        ) isEqualTo [];
    }
    else {
        if (_raycast isEqualType objNull) then {
            private _result = false;
            private _checkerObject = createSimpleObject ["KH_HelperRectangle_1x1x2", _end, true];
            _checkerObject setVectorDirAndUp [[0, 1, 0], [0, 0, 1]];
            _checkerObject setPhysicsCollisionFlag false;
            private _raycasts = [];

            for "_positionX" from -0.5 to 0.5 step 0.5 do {
                for "_positionY" from -0.5 to 0.5 step 0.5 do {
                    for "_positionZ" from 0 to 2 step 0.5 do {
                        _raycasts pushBack [
                            _start,
                            _end vectorAdd [_positionX, _positionY, _positionZ],
                            _ignored,
                            true,
                            1,
                            "VIEW",
                            "NONE",
                            true,
                            []
                        ]
                    };
                };
            };

            {
                if ((_x select 3) isEqualTo _checkerObject) then {
                    result = true;
                    break;
                };
            } forEach ([_raycasts] call KH_fnc_raycast);

            deleteVehicle _checkerObject;
            _result;
        }
        else {
            true;
        };
    };
};

if (_direction isEqualType objNull) then {
    if (isPlayer _direction) then {
        _direction = _direction getVariable ["KH_var_playerCameraDirection", eyeDirection _direction];
    }
    else {
        if (_direction isKindOf "Man") then {
            _direction = eyeDirection _direction;
        }
        else {
            _direction = vectorDir _direction;
        };
    };
};

private _toTarget = _end vectorDiff _start;
private _toTargetNormalized = _toTarget vectorMultiply (1 / ((vectorMagnitude _toTarget) max 0.001));
private _directionXY = [_direction select 0, _direction select 1, 0];
private _targetXY = [_toTargetNormalized select 0, _toTargetNormalized select 1, 0];
private _directionXYMagnitude = (vectorMagnitude _directionXY) max 0.001;
private _targetXYMagnitude = (vectorMagnitude _targetXY) max 0.001;
private _horizontalAngle = 90;

if (_directionXYMagnitude > 0.001 && _targetXYMagnitude > 0.001) then {
    _directionXY = _directionXY vectorMultiply (1 / _directionXYMagnitude);
    _targetXY = _targetXY vectorMultiply (1 / _targetXYMagnitude);
    private _dotProduct = _directionXY vectorDotProduct _targetXY;
    _dotProduct = (_dotProduct max -1) min 1;
    _horizontalAngle = acos _dotProduct;
};

private _dirPitch = asin (((_direction select 2) max 0.001) / ((vectorMagnitude _direction) max 0.001));
private _targetPitch = asin (_toTargetNormalized select 2);
private _verticalAngle = abs (_targetPitch - _dirPitch);

if !((_horizontalAngle <= (_horizontalFov / 2)) && (_verticalAngle <= (_verticalFov / 2))) exitWith {
    false;
};

if (_raycast isNotEqualTo false) then {
    if (_raycast isEqualTo true) then {
        (
            [
                _start,
                _end,
                _ignored,
                true,
                1,
                "VIEW",
                "NONE",
                true,
                []
            ] call KH_fnc_raycast
        ) isEqualTo [];
    }
    else {
        private _result = false;
        private _checkerObject = createSimpleObject ["KH_HelperRectangle_1x1x2", _end, true];
        _checkerObject setVectorDirAndUp [[0, 1, 0], [0, 0, 1]];
        _checkerObject setPhysicsCollisionFlag false;
        private _raycasts = [];

        for "_positionX" from -0.5 to 0.5 step 0.5 do {
            for "_positionY" from -0.5 to 0.5 step 0.5 do {
                for "_positionZ" from 0 to 2 step 0.5 do {
                    _raycasts pushBack [
                        _start,
                        _end vectorAdd [_positionX, _positionY, _positionZ],
                        _ignored,
                        true,
                        1,
                        "VIEW",
                        "NONE",
                        true,
                        []
                    ]
                };
            };
        };

        {
            if ((_x select 3) isEqualTo _checkerObject) then {
                result = true;
                break;
            };
        } forEach ([_raycasts] call KH_fnc_raycast);

        deleteVehicle _checkerObject;
        _result;
    };
}
else {
    true;
};