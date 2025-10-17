params [
	["_position", [0, 0, 0], [[], objNull]],
    ["_vectorDirAndUp", [[0, 1, 0], [0, 0, 1]], [[], objNull]],
    ["_length", 1, [0]],
    ["_angle", 0, [0]],
    ["_cone", 0, [0]],
	["_step", 0.1, [0]],
	["_ignored", [], [[]]],
	["_sort", true, [true]],
	["_maxResults", -1, [0]],
	["_lod1", "VIEW", [""]], 
	["_lod2", "FIRE", [""]], 
	["_returnUnique", true, [true]],
	["_draw", [], [[]]]
];

_angle = ((_angle min 180) max 0) - 90;
private _allowIgnoredCheck = true;
private _ignored1 = objNull;
private _ignored2 = objNull;

if ((count _ignored) <= 2) then {
	_ignored1 = _ignored param [0, objNull, [objNull]];
	_ignored2 = _ignored param [1, objNull, [objNull]];
	_allowIgnoredCheck = false;
};

if (_position isEqualType objNull) then {
	_position = _position modelToWorldVisualWorld [0, 0, 0];
}
else {
	if ((_position select 0) isEqualType objNull) then {
		_position = (_position select 0) modelToWorldVisualWorld (_position select 1);
	};
};

private _grids = [];
private _intersectionCheckCount = 0;
private _maxResultsOverride = [_maxResults, -1] select _allowIgnoredCheck;

if (_vectorDirAndUp isEqualType objNull) then {
    _vectorDirAndUp = [vectorDir _vectorDirAndUp, vectorUp _vectorDirAndUp];
};

_vectorDirAndUp params [["_vectorDir", [0, 1, 0], [[]]], ["_vectorUp", [0, 0, 1], [[]]]];

if (_cone isEqualTo 0) then {
    private _right = vectorNormalized (_vectorDir vectorCrossProduct _vectorUp);
    private _rayCount = ceil (360 / (_step * 10));
    private _componentZ = (sin _angle) * _length;
    private _horizontalRadius = (cos _angle) * _length;

    for "_currentAngleStep" from 0 to (_rayCount - 1) do {
        private _azimuth = _currentAngleStep * (360 / _rayCount);
        private _localX = (cos _azimuth) * _horizontalRadius;
        private _localY = (sin _azimuth) * _horizontalRadius;

        _grids pushBack [
            _position, 
            _position vectorAdd [
                (_right select 0) * _localX + (_vectorDir select 0) * _localY + (_vectorUp select 0) * _componentZ,
                (_right select 1) * _localX + (_vectorDir select 1) * _localY + (_vectorUp select 1) * _componentZ,
                (_right select 2) * _localX + (_vectorDir select 2) * _localY + (_vectorUp select 2) * _componentZ
            ], 
            _ignored1, 
            _ignored2, 
            _sort, 
            _maxResultsOverride, 
            _lod1, 
            _lod2, 
            _returnUnique
        ];

        _intersectionCheckCount = _intersectionCheckCount + 1;
    };
}
else {
    private _right = vectorNormalized (_vectorDir vectorCrossProduct _vectorUp);
    private _rayCount = ceil (_cone / (_step * 10));

    if (_rayCount < 2) then { 
        _rayCount = 2; 
    };
    
    private _cosElevation = cos _angle;
    private _sinElevation = sin _angle;
    
    for "_currentAngleStep" from 0 to (_rayCount - 1) do {
        private _rayAngle = -(_cone / 2) + (_currentAngleStep / (_rayCount - 1)) * _cone;
        private _cosAngle = cos _rayAngle;
        private _sinAngle = sin _rayAngle;
        
        private _horizontalDirection = [
            (_vectorDir select 0) * _cosAngle + (_right select 0) * _sinAngle,
            (_vectorDir select 1) * _cosAngle + (_right select 1) * _sinAngle,
            (_vectorDir select 2) * _cosAngle + (_right select 2) * _sinAngle
        ];
        
        private _rayDirection = [
            (_horizontalDirection select 0) * _cosElevation + (_vectorUp select 0) * _sinElevation,
            (_horizontalDirection select 1) * _cosElevation + (_vectorUp select 1) * _sinElevation,
            (_horizontalDirection select 2) * _cosElevation + (_vectorUp select 2) * _sinElevation
        ];
        
        _grids pushBack [
            _position,
            _position vectorAdd (_rayDirection vectorMultiply _length),
            _ignored1,
            _ignored2,
            _sort,
            _maxResultsOverride,
            _lod1,
            _lod2,
            _returnUnique
        ];
        
        _intersectionCheckCount = _intersectionCheckCount + 1;
    };
};

private _intersections = lineIntersectsSurfaces [_grids];
private _intersectionCount = count (_intersections select {_x isNotEqualTo [];});
private _flattenedIntersections = [];

{
	{
		_flattenedIntersections pushBack _x;
	} forEach _x;
} forEach _intersections;

_intersections = _flattenedIntersections;

if _allowIgnoredCheck then {
	private _deletions = [];
	private _resultCount = [0, 999999] select (_maxResults isEqualTo -1);
	private _ignoreTerrain = "TERRAIN" in _ignored;
	private _ignoredObjects = _ignored select {_x isEqualType objNull;};
	private _ignoredTypes = _ignored select {_x isEqualType "";};

	{		
		if (_resultCount >= _maxResults) then {
			_deletions pushBack _forEachIndex;
			continue;
		};

		private _object = _x param [3, objNull];

		if ((isNull _object) && _ignoreTerrain) then {
			_deletions pushBack _forEachIndex;
		}
		else {
			if ((_object in _ignoredObjects) || ((_ignoredTypes findIf {_object isKindOf _x;}) isNotEqualTo -1)) then {
				_deletions pushBack _forEachIndex;
			}
			else {
				_resultCount = _resultCount + 1;
			};
		};
	} forEach _intersections;

	_intersections deleteAt _deletions;
};

if (_draw isNotEqualTo []) then {
	_draw params [["_type", "", [""]], ["_arguments", [], [[]]], ["_duration", 0, [0, ""]]];

	if (_duration isEqualTo 0) then {
		_duration = true;
	};

	switch _type do {
		case "LINE": {
			_arguments params [["_color", [1, 1, 1, 1], [[]]], ["_width", 1, [0]]];

			{
				[_type, [ASLToAGL (_x select 0), ASLToAGL (_x select 1), _color, _width], _duration, [], {}] call KH_fnc_draw3d;
			} forEach _grids;
		};

		case "LASER": {
			_arguments params [["_beamColor", [1, 1, 1], [[]]], ["_dotColor", [1, 1, 1], [[]]], ["_dotSize", 1, [0]], ["_beamThickness", 1, [0]], ["_beamMaxLength", 0, [0]], ["_ir", false, [true]]];

			{
				[_type, [_x select 0, _x select 1, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir], _duration, [], {}] call KH_fnc_draw3d;
			} forEach _grids;
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
					[_texture, _color, ASLToAGL (_x select 1), _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY], 
					_duration, 
					[], 
					{}
				] call KH_fnc_draw3d;
            } forEach _grids;
		};
	};
};

[_intersections, _intersectionCheckCount, _intersectionCount];