if ((count _this) isEqualTo 1) then {
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

params [["_start", [0, 0, 0], [[], objNull]], ["_end", [0, 0, 0], [[], objNull]], ["_direction", [0, 1, 0], [[], objNull]], ["_fov", 360, [0, []]], ["_limitDistance", 0, [0]], ["_raycast", false, [true, objNull]]];
private ["_horizontalFov", "_verticalFov"];
private _minimalFov = 0;

if (_fov isEqualType []) then {
    _horizontalFov = abs (_fov select 0);
    _verticalFov = abs (_fov select 1);
    _fov = (abs (_fov select 0)) min (abs (_fov select 1));
}
else {
    _minimalFov = abs _fov;
};

private _ignored = if ((_start isEqualType objNull) && (_raycast isNotEqualTo false)) then {
    [_start, objectParent _start, attachedTo _start] + (attachedObjects _start);
}
else {
    [];
};

if (_start isEqualType objNull) then {
    if (isPlayer _start) then {
        if (_fov isEqualType 0) then {
            _verticalFov = (getObjectFOV _start) * (180 / pi);
            _horizontalFov = 2 * (atan (((_start getVariable ["KH_var_playerAspectRatio", 1.777777]) * _fov) * tan ((_verticalFov / 2) * (pi / 180)))) * (180 / pi);
        }
        else {
            _horizontalFov = _fov;
            _verticalFov = _fov;
        };
        
        _start = AGLToASL (_start getVariable ["KH_var_playerCameraPosition", eyePos _start]);
    }
    else {
        _horizontalFov = _fov;
        _verticalFov = _fov;

        if (_start isKindOf "Man") then {
            _start = eyePos _start;
        }
        else {
            _start = getPosASL _start;
        };
    };
};

if (_end isEqualType objNull) then {
    _end = getPosASL _end;
};

if (_limitDistance isEqualTo 0) then {
    _limitDistance = 999999;
};

if ((_start vectorDistance _end) > _limitDistance) exitWith {
    false;
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
                "FIRE",
                true,
                []
            ] call KH_fnc_raycast
        ) isNotEqualTo [];
    }
    else {
        if (_raycast isEqualType objNull) then {
            private _checkerObject = createSimpleObject ["KH_HelperRectangle_1x1x2", _end, true];
            _checkerObject setVectorDirAndUp [[0, 1, 0], [0, 0, 1]];
            _checkerObject setPhysicsCollisionFlag false;
            private _raycasts = [];

            for "_positionX" from -0.5 to 0.5 step 0.25 do {
                for "_positionY" from -0.5 to 0.5 step 0.25 do {
                    for "_positionZ" from 0 to 2 step 0.5 do {
                        _raycasts pushBack [
                            _start,
                            _end vectorAdd [_positionX, _positionY, _positionZ],
                            _ignored,
                            true,
                            1,
                            "VIEW",
                            "FIRE",
                            true,
                            []
                        ]
                    };
                };
            };

            ([_raycasts] call KH_fnc_raycast) isNotEqualTo [];
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
private _toTargetNormalized = _toTarget vectorMultiply (1 / (vectorMagnitude _toTarget));
private _directionXY = [_direction select 0, _direction select 1, 0];
private _targetXY = [_toTargetNormalized select 0, _toTargetNormalized select 1, 0];
private _directionXYMagnitude = vectorMagnitude _directionXY;
private _targetXYMagnitude = vectorMagnitude _targetXY;
private _horizontalAngle = 90;

if (_directionXYMagnitude > 0.001 && _targetXYMagnitude > 0.001) then {
    _directionXY = _directionXY vectorMultiply (1 / _directionXYMagnitude);
    _targetXY = _targetXY vectorMultiply (1 / _targetXYMagnitude);
    private _dotProduct = _directionXY vectorDotProduct _targetXY;
    _dotProduct = (_dotProduct max -1) min 1;
    _horizontalAngle = acos _dotProduct;
};

private _dirPitch = asin ((_direction select 2) / (vectorMagnitude _direction));
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
                "FIRE",
                true,
                []
            ] call KH_fnc_raycast
        ) isNotEqualTo [];
    }
    else {
        private _checkerObject = createSimpleObject ["KH_HelperRectangle_1x1x2", _end, true];
        _checkerObject setVectorDirAndUp [[0, 1, 0], [0, 0, 1]];
        _checkerObject setPhysicsCollisionFlag false;
        private _raycasts = [];

        for "_positionX" from -0.5 to 0.5 step 0.25 do {
            for "_positionY" from -0.5 to 0.5 step 0.25 do {
                for "_positionZ" from 0 to 2 step 0.5 do {
                    _raycasts pushBack [
                        _start,
                        _end vectorAdd [_positionX, _positionY, _positionZ],
                        _ignored,
                        true,
                        1,
                        "VIEW",
                        "FIRE",
                        true,
                        []
                    ]
                };
            };
        };

        ([_raycasts] call KH_fnc_raycast) isNotEqualTo [];
    };
}
else {
    true;
};