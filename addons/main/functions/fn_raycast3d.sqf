params [
	["_position", [0, 0, 0], [[], objNull]],
	["_dimensions", [1, "1", 1], [[]]], 
	["_type", "RECTANGLE", [""]], 
	["_step", 0.1, [0]], 
	["_ignored", [], [[]]],
	["_sort", true, [true]],
	["_maxResults", -1, [0]],
	["_lod1", "VIEW", [""]], 
	["_lod2", "FIRE", [""]], 
	["_returnUnique", true, [true]],
	["_draw", [], [[]]]
];

private _allowIgnoredCheck = true;
private _ignored1 = objNull;
private _ignored2 = objNull;

if ((count _ignored) <= 2) then {
	_ignored1 = _ignored param [0, objNull, [objNull]];
	_ignored2 = _ignored param [1, objNull, [objNull]];
	_allowIgnoredCheck = false;
};

private "_object";

if (_position isEqualType objNull) then {
	_object = _position;
	_position = _object modelToWorldVisualWorld [0, 0, 0];
}
else {
	if ((_position select 0) isEqualType objNull) then {
		_object = _position select 0;
		private _relativePosition = _position select 1;
		_position = _object modelToWorldVisualWorld [-(_relativePosition select 0), -(_relativePosition select 1), -(_relativePosition select 2)];
	};
};

private "_primaryAxis";
private "_primaryAxisIndex";
private "_secondaryAxis";
private "_secondaryAxisIndex";
private "_tertiaryAxis";
private "_tertiaryAxisIndex";

{
	if (_x isEqualType "") then {
		_primaryAxis = (parseNumber _x) min 5000;
		_primaryAxisIndex = _forEachIndex;
	}
	else {
		if (isNil "_secondaryAxis") then {
			_secondaryAxis = abs _x;
			_secondaryAxisIndex = _forEachIndex;
		}
		else {
			_tertiaryAxis = abs _x;
			_tertiaryAxisIndex = _forEachIndex;
		};
	};
} forEach _dimensions;

private _grids = [];
private _intersectionCheckCount = 0;
private _maxResultsOverride = [_maxResults, -1] select _allowIgnoredCheck;

switch _type do {
	case "RECTANGLE": {
		for "_secondaryAxisIteration" from -_secondaryAxis to _secondaryAxis step _step do {
			for "_tertiaryAxisIteration" from -_tertiaryAxis to _tertiaryAxis step _step do {
				private _currentPositionStart = [];
				private _currentPositionEnd = [];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxis];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, -_primaryAxis];
				_currentPositionEnd set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, _tertiaryAxisIteration];

				if !(isNil "_object") then {
					private _relativeOffsetStart = _object modelToWorldVisualWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldVisualWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResultsOverride, _lod1, _lod2, _returnUnique];
				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};
	
	case "OVAL": {
		private "_secondaryAxisIteration";
		private "_tertiaryAxisIteration";
		private "_primaryAxisIteration";
		
		for "_phi" from -90 to 90 step (_step * 20) do {
			for "_theta" from 0 to 360 step (_step * 20) do {
				_secondaryAxisIteration = _secondaryAxis * (cos _phi) * (cos _theta);
				_tertiaryAxisIteration = _tertiaryAxis * (cos _phi) * (sin _theta);
				_primaryAxisIteration = _primaryAxis * (sin _phi);
				private _currentPositionStart = [];
				private _currentPositionEnd = [];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxisIteration];
				_currentPositionEnd set [_secondaryAxisIndex, -_secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, -_tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, -_primaryAxisIteration];
				
				if !(isNil "_object") then {
					private _relativeOffsetStart = _object modelToWorldVisualWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldVisualWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResultsOverride, _lod1, _lod2, _returnUnique];
				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};

	case "CYLINDER": {
		private "_secondaryAxisIteration";
		private "_tertiaryAxisIteration";
		private "_primaryAxisIteration";
		
		for "_theta" from 0 to 360 step (_step * 20) do {
			for "_currentHeight" from -_primaryAxis to _primaryAxis step _step do {
				_secondaryAxisIteration = _secondaryAxis * (cos _theta);
				_tertiaryAxisIteration = _tertiaryAxis * (sin _theta);
				_primaryAxisIteration = -_currentHeight;
				private _currentPositionStart = [];
				private _currentPositionEnd = [];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxisIteration];
				_currentPositionEnd set [_secondaryAxisIndex, -_secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, -_tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, _primaryAxisIteration];
				
				if !(isNil "_object") then {
					private _relativeOffsetStart = _object modelToWorldVisualWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldVisualWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResultsOverride, _lod1, _lod2, _returnUnique];
				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};

	case "CONE": {
		private "_radiusRatio";
		private "_maxSecondaryRadius";
		private "_maxTertiaryRadius";
		private "_maxRadius";
		private "_currentRadiusScale";
		private "_surfaceSecondary";
		private "_surfaceTertiary";
		
		for "_theta" from 0 to 360 step (_step * 20) do {
			for "_currentHeight" from 0 to _primaryAxis step _step do {
				_radiusRatio = _currentHeight / _primaryAxis;
				_maxSecondaryRadius = _secondaryAxis * _radiusRatio;
				_maxTertiaryRadius = _tertiaryAxis * _radiusRatio;
				_maxRadius = _maxSecondaryRadius max _maxTertiaryRadius;
				
				for "_currentRadiusOffset" from 0 to _maxRadius step (_step * 2) do {
					_currentRadiusScale = if (_maxRadius > 0) then { 
						_currentRadiusOffset / _maxRadius;
					} 
					else { 
						0;
					};

					_surfaceSecondary = _maxSecondaryRadius * _currentRadiusScale * (cos _theta);
					_surfaceTertiary = _maxTertiaryRadius * _currentRadiusScale * (sin _theta);
					private _currentPositionStart = [];
					private _currentPositionEnd = [];
					_currentPositionStart set [_secondaryAxisIndex, 0];
					_currentPositionStart set [_tertiaryAxisIndex, 0];
					_currentPositionStart set [_primaryAxisIndex, 0];
					_currentPositionEnd set [_secondaryAxisIndex, _surfaceSecondary];
					_currentPositionEnd set [_tertiaryAxisIndex, _surfaceTertiary];
					_currentPositionEnd set [_primaryAxisIndex, _currentHeight];
					
					if !(isNil "_object") then {
						private _relativeOffsetStart = _object modelToWorldVisualWorld _currentPositionStart;
						_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
						private _relativeOffsetEnd = _object modelToWorldVisualWorld _currentPositionEnd;
						_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
					}
					else {
						_currentPositionStart = _position vectorAdd _currentPositionStart;
						_currentPositionEnd = _position vectorAdd _currentPositionEnd;
					};

					_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResultsOverride, _lod1, _lod2, _returnUnique];
					_intersectionCheckCount = _intersectionCheckCount + 1;
				};
			};
		};
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
				[_type, [_texture, _color, ASLToAGL (_x select 1), _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY], _duration, [], {}] call KH_fnc_draw3d;
			} forEach _grids;
		};
	};
};

[_intersections, _intersectionCheckCount, _intersectionCount];