if ((count _this) isEqualTo 1) then {
	params [["_intersectionArguments", [], [[]]]];
	private _allIntersections = [];

	{
		_x params [
			["_start", [0, 0, 0], [[], objNull]],
			["_end", [1, 1, 1], [[], objNull]],
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

		if (_start isEqualType objNull) then {
			_start = _start modelToWorldVisualWorld [0, 0, 0];
		}
		else {
			if ((_start select 0) isEqualType objNull) then {
				_start = (_start select 0) modelToWorldVisualWorld (_start select 1);
			};
		};

		if (_end isEqualType objNull) then {
			_end = _end modelToWorldVisualWorld [0, 0, 0];
		}
		else {
			if ((_end select 0) isEqualType objNull) then {
				_end = (_end select 0) modelToWorldVisualWorld (_end select 1);
			};
		};

		private _distance = _start vectorDistance _end;

		if (_distance <= 5000) then {
			_allIntersections pushBack [
				_start, 
				_end, 
				_ignored1, 
				_ignored2, 
				_sort,
				[_maxResults, -1] select _allowIgnoredCheck, 
				_lod1, 
				_lod2, 
				_returnUnique
			];
		}
		else {
			private _maxResultsOverride = [_maxResults, -1] select _allowIgnoredCheck;

			for "_i" from 1 to (ceil (_distance / 5000)) do {
				private _currentDistance = _distance - (_i - 1) * 5000;

				if (_currentDistance > 5000) then { 
					_currentDistance = 5000;
				};

				private _currentStart = _start vectorAdd ((_start vectorFromTo _end) vectorMultiply ((_i - 1) * 5000));

				_allIntersections pushBack [
					_currentStart, 
					_currentStart vectorAdd ((_start vectorFromTo _end) vectorMultiply _currentDistance), 
					_ignored1, 
					_ignored2, 
					_sort, 
					_maxResultsOverride, 
					_lod1,
					_lod2, 
					_returnUnique
				];
			};
		};
	} forEach _intersectionArguments;

	private _intersections = lineIntersectsSurfaces [_allIntersections];
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
				[_type, [ASLToAGL _start, ASLToAGL _end, _color, _width], _duration, [], {}] call KH_fnc_draw3d;
			};

			case "LASER": {
				_arguments params [["_beamColor", [1, 1, 1], [[]]], ["_dotColor", [1, 1, 1], [[]]], ["_dotSize", 1, [0]], ["_beamThickness", 1, [0]], ["_beamMaxLength", 0, [0]], ["_ir", false, [true]]];
				[_type, [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir], _duration, [], {}] call KH_fnc_draw3d;
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

				[_type, [_texture, _color, ASLToAGL _end, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY], _duration, [], {}] call KH_fnc_draw3d;
			};
		};
	};

	_intersections;
}
else {
	params [
		["_start", [0, 0, 0], [[], objNull]],
		["_end", [1, 1, 1], [[], objNull]],
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

	if (_start isEqualType objNull) then {
		_start = _start modelToWorldVisualWorld [0, 0, 0];
	}
	else {
		if ((_start select 0) isEqualType objNull) then {
			_start = (_start select 0) modelToWorldVisualWorld (_start select 1);
		};
	};

	if (_end isEqualType objNull) then {
		_end = _end modelToWorldVisualWorld [0, 0, 0];
	}
	else {
		if ((_end select 0) isEqualType objNull) then {
			_end = (_end select 0) modelToWorldVisualWorld (_end select 1);
		};
	};

	private _distance = _start vectorDistance _end;
	private _multipleIntersections = false;

	private _intersections = if (_distance <= 5000) then {
		lineIntersectsSurfaces [
			_start, 
			_end, 
			_ignored1, 
			_ignored2,
			_sort, 
			[_maxResults, -1] select _allowIgnoredCheck, 
			_lod1, 
			_lod2, 
			_returnUnique
		];
	}
	else {
		private _allIntersections = [];
		private _maxResultsOverride = [_maxResults, -1] select _allowIgnoredCheck;

		for "_i" from 1 to (ceil (_distance / 5000)) do {
			private _currentDistance = _distance - (_i - 1) * 5000;

			if (_currentDistance > 5000) then { 
				_currentDistance = 5000;
			};

			private _currentStart = _start vectorAdd ((_start vectorFromTo _end) vectorMultiply ((_i - 1) * 5000));

			_allIntersections pushBack [
				_currentStart, 
				_currentStart vectorAdd ((_start vectorFromTo _end) vectorMultiply _currentDistance), 
				_ignored1, 
				_ignored2, 
				_sort, 
				_maxResultsOverride, 
				_lod1,
				_lod2, 
				_returnUnique
			];
		};

		_multipleIntersections = true;
		lineIntersectsSurfaces [_allIntersections];
	};

	if _multipleIntersections then {
		private _flattenedIntersections = [];

		{
			{
				_flattenedIntersections pushBack _x;
			} forEach _x;
		} forEach _intersections;

		_intersections = _flattenedIntersections;
	};

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
					[_type, [ASLToAGL _start, ASLToAGL _end, _color, _width], _duration, [], {}] call KH_fnc_draw3d;
				} forEach _grids;
			};

			case "LASER": {
				_arguments params [["_beamColor", [1, 1, 1], [[]]], ["_dotColor", [1, 1, 1], [[]]], ["_dotSize", 1, [0]], ["_beamThickness", 1, [0]], ["_beamMaxLength", 0, [0]], ["_ir", false, [true]]];

				{
					[_type, [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir], _duration, [], {}] call KH_fnc_draw3d;
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
					[_type, [_texture, _color, ASLToAGL _end, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY], _duration, [], {}] call KH_fnc_draw3d;
				} forEach _grids;
			};
		};
	};

	_intersections;
};