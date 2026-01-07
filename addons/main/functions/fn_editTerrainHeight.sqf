params [
    ["_position", [0, 0, 0], [[], objNull]],
    ["_radius", 0, [0, []]],
    ["_shape", true, [true]],
    ["_direction", 0, [0, objNull]],
    ["_curve", [], [[]]],
    ["_applicationType", "ADD", [""]],
    ["_curveType", "LINEAR", [""]],
    ["_changeDuration", 0, [0, []]],
    ["_adjustObjectHeight", true, [true]]
];

if (isNil "KH_var_allModifiedTerrain") then {
    KH_var_allModifiedTerrain = createHashMap;
};

if (_curve isEqualTo []) exitWith {
    [];
};

if (_radius isEqualType 0) then {
    _radius = [_radius, _radius];
};

_radius params [["_radiusA", 0, [0]], ["_radiusB", 0, [0]]];

if ((_radiusA isEqualTo 0) || (_radiusB isEqualTo 0)) exitWith {
    [];
};

if (_position isEqualType objNull) then {
    _position = getPosASL _position;
};

if (_direction isEqualType objNull) then {
    _direction = getDir _direction;
};

private "_animationCurveType";

if (_changeDuration isEqualType []) then {
    _animationCurveType = _changeDuration param [1, "LINEAR", [""]];
    _changeDuration = _changeDuration param [0, 0, [0]];
}
else {
    _animationCurveType = "LINEAR";
};

private _gridWidth = getTerrainInfo select 2;
private _snappedX = round ((_position select 0) / _gridWidth) * _gridWidth;
private _snappedY = round ((_position select 1) / _gridWidth) * _gridWidth;
private _cosDirection = cos _direction;
private _sinDirection = sin _direction;
private _sortedCurve = [_curve, [], {(_x select 0) select 0;}] call BIS_fnc_sortBy;
private _maxRadius = _radiusA max _radiusB;
private _gridSteps = ceil (_maxRadius / _gridWidth);
private _minX = _snappedX - (_gridSteps * _gridWidth);
private _maxX = _snappedX + (_gridSteps * _gridWidth);
private _minY = _snappedY - (_gridSteps * _gridWidth);
private _maxY = _snappedY + (_gridSteps * _gridWidth);
private _originalHeights = [];
private _targetHeights = [];

for "_i" from _minX to _maxX step _gridWidth do {
    for "_j" from _minY to _maxY step _gridWidth do {
        private _dx = _i - _snappedX;
        private _dy = _j - _snappedY;
        private _rotatedX = _dx * _cosDirection + _dy * _sinDirection;
        private _rotatedY = -_dx * _sinDirection + _dy * _cosDirection;
        private _inShape = false;
        private _normalizedDistance = 0;

        if _shape then {
            if (((abs _rotatedX) <= _radiusA) && ((abs _rotatedY) <= _radiusB)) then {
                _inShape = true;

                private _normX = if (_radiusA > 0) then { 
                    abs (_rotatedX / _radiusA);
                } 
                else { 
                    0;
                };

                private _normY = if (_radiusB > 0) then { 
                    abs (_rotatedY / _radiusB);
                } 
                else { 
                    0;
                };

                _normalizedDistance = 1 - (_normX max _normY);
            };
        }
        else {
            private _ellipseDistance = ((_rotatedX / _radiusA) ^ 2) + ((_rotatedY / _radiusB) ^ 2);

            if (_ellipseDistance <= 1) then {
                _inShape = true;
                _normalizedDistance = 1 - (sqrt _ellipseDistance);
            };
        };

        if _inShape then {
            private _heightMod = if (_sortedCurve isEqualTo []) then {
                0;
            }
            else {
                private _points = _sortedCurve apply {
                    _x params ["_range", "_height"];
                    _range params ["_min", "_max"];
                    [(_min + _max) / 2, _height]
                };

                private _index = _points findIf { 
                    (_x select 0) > _normalizedDistance;
                };

                if (_index isEqualTo -1) then {
                    _index = (count _points) - 1;
                }
                else {
                    _index = _index max 1;
                };

                _index = _index min (count _points - 1);
                private _p1 = _points select (_index - 1);
                private _p2 = _points select _index;
                
                private _t = if ((_p2 select 0) isEqualTo (_p1 select 0)) then { 
                    0;
                } 
                else { 
                    (((_normalizedDistance - (_p1 select 0)) / ((_p2 select 0) - (_p1 select 0))) max 0) min 1;
                };
                
                switch _curveType do {
                    case "NONE": { 
                        _p1 select 1;
                    };
                    
                    case "LINEAR": { 
                        linearConversion [0, 1, _t, _p1 select 1, _p2 select 1, false];
                    };
                    
                    case "BEZIER": {
                        private _p0 = _points select ((_index - 2) max 0);
                        private _p3 = _points select ((_index + 1) min (count _points - 1));
                        private _h0 = _p0 select 1;
                        private _h1 = _p1 select 1;
                        private _h2 = _p2 select 1;
                        private _h3 = _p3 select 1;
                        private _t2 = _t * _t;
                        private _t3 = _t2 * _t;
                        0.5 * ((2 * _h1) + (-_h0 + _h2) * _t + (2 * _h0 - 5 * _h1 + 4 * _h2 - _h3) * _t2 + (-_h0 + 3 * _h1 - 3 * _h2 + _h3) * _t3);
                    };
                };
            };

            private _currentHeight = getTerrainHeightASL [_i, _j];

            private _newHeight = switch _applicationType do {
                case "ADD": { 
                    _currentHeight + _heightMod;
                };

                case "MULTIPLY": { 
                    _currentHeight * _heightMod;
                };

                case "SET": { 
                    _heightMod;
                };

                default { 
                    _currentHeight;
                };
            };

            if (_newHeight isNotEqualTo _currentHeight) then {
                _originalHeights pushBack [_i, _j, _currentHeight];
                _targetHeights pushBack [_i, _j, _newHeight];
            };
        };
    };
};

if (_targetHeights isEqualTo []) exitWith {
    [];
};

if (_changeDuration isEqualTo 0) then {
    setTerrainHeight [_targetHeights, _adjustObjectHeight];
}
else {
    [
        [_originalHeights, _targetHeights, _changeDuration, _animationCurveType, _adjustObjectHeight, diag_tickTime],
        {
            params ["_originalHeights", "_targetHeights", "_changeDuration", "_animationCurveType", "_adjustObjectHeight", "_startTime"];

            if (diag_tickTime < (_startTime + _changeDuration)) then {
                private _time = (diag_tickTime - _startTime) / _changeDuration;
                private _interpolatedHeights = [];

                {
                    _x params ["_i", "_j", "_originalHeight"];
                    
                    _interpolatedHeights pushBack [
                        _i, 
                        _j, 
                        switch _animationCurveType do {
                            case "NONE": { 
                                _originalHeight;
                            };

                            case "LINEAR": { 
                                linearConversion [0, 1, _time, _originalHeight, (_targetHeights select _forEachIndex) select 2, false];
                            };

                            case "BEZIER": {
                                linearConversion [0, 1, _time * _time * (3 - 2 * _time), _originalHeight, (_targetHeights select _forEachIndex) select 2, false];
                            };
                        }
                    ];

                } forEach _originalHeights;

                setTerrainHeight [_interpolatedHeights, _adjustObjectHeight];
            }
            else {
                setTerrainHeight [_targetHeights, _adjustObjectHeight];
                [_handlerId] call KH_fnc_removeHandler;
            };
        },
        true,
        0,
        false
    ] call KH_fnc_execute;
};

{
    KH_var_allModifiedTerrain set [[_x select 0, _x select 1], _x select 2, true];
} forEach _originalHeights;

_originalHeights;