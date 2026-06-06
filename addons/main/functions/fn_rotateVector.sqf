params [["_pivot", [0, 0, 0], [[], objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_vectorDirAndUp", [[0, 1, 0], [0, 0, 1]], [[], objNull]]];

if (_pivot isEqualType objNull) then {
    _pivot = getPosASLVisual _pivot;
};

if (_position isEqualType objNull) then {
    _position = getPosASLVisual _position;
};

if (_vectorDirAndUp isEqualType objNull) then {
    _vectorDirAndUp = [vectorDirVisual _vectorDirAndUp, vectorUpVisual _vectorDirAndUp];
}
else {
    if (_vectorDirAndUp isEqualTypeAll 0) then {
        _vectorDirAndUp = eulerToVector _vectorDirAndUp;
    };
};

_vectorDirAndUp params [["_vectorDir", [0, 1, 0], [[]]], ["_vectorUp", [0, 0, 1], [[]]]];
private _vectorSide = _vectorDir vectorCrossProduct _vectorUp;
private _offset = _position vectorDiff _pivot;

private _result = [
    [_vectorSide select 0, _vectorDir select 0, _vectorUp select 0],
    [_vectorSide select 1, _vectorDir select 1, _vectorUp select 1],
    [_vectorSide select 2, _vectorDir select 2, _vectorUp select 2]
] matrixMultiply [
    [_offset select 0],
    [_offset select 1],
    [_offset select 2]
];

_pivot vectorAdd [
    (_result select 0) select 0,
    (_result select 1) select 0,
    (_result select 2) select 0
];