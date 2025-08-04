params [
	["_position", [], [[]]],
	["_dimensions", [], [[]]], 
	["_type", "RECTANGULAR", [""]], 
	["_step", 0.1, [0]], 
	["_draw", [], [[]]],
	["_ignoredObjects", [], [[]]],
	["_ignoredTypes", [], [[]]],
	["_sort", false, [true]], 
	["_lod1", "VIEW", [""]], 
	["_lod2", "GEOM", [""]], 
	["_returnUnique", true, [true]],
	["_verboseRays", false, [true]]
];

private _object = objNull;
private _temporaryObject = false;

if ((_position select 0) isEqualType objNull) then {
	if ((_position select 1) isEqualType []) then {
		_object = _position select 0;

		if (((_position select 1) select 0) isEqualType []) then {
			private _relativePosition = (_position select 1) select 0;
			_relativePosition = _object modelToWorldWorld [-(_relativePosition select 0), -(_relativePosition select 1), -(_relativePosition select 2)];
			private _relativePositionOffset = (_position select 1) select 1;
			_relativePositionOffset = _object modelToWorldWorld [-(_relativePositionOffset select 0), -(_relativePositionOffset select 1), -(_relativePositionOffset select 2)];
			private _finalPosition = _relativePosition vectorAdd _relativePositionOffset;
			_position = _object modelToWorldWorld [-(_finalPosition select 0), -(_finalPosition select 1), -(_finalPosition select 2)];
		}
		else {
			private _relativePosition = _position select 1;
			_position = _object modelToWorldWorld [-(_relativePosition select 0), -(_relativePosition select 1), -(_relativePosition select 2)];
		};
	}
	else {
		_temporaryObject = true;
		_object = createVehicleLocal ["KH_HelperSquare", [0, 0, 0], [], 0, "CAN_COLLIDE"];
		_object attachTo [_position select 0, [0, 0, 0], _position select 1, true];

		if (((_position select 2) select 0) isEqualType []) then {
			private _relativePosition = (_position select 2) select 0;
			_relativePosition = _object modelToWorldWorld [-(_relativePosition select 0), -(_relativePosition select 1), -(_relativePosition select 2)];
			private _relativePositionOffset = (_position select 2) select 1;
			_relativePositionOffset = _object modelToWorldWorld [-(_relativePositionOffset select 0), -(_relativePositionOffset select 1), -(_relativePositionOffset select 2)];
			private _finalPosition = _relativePosition vectorAdd _relativePositionOffset;
			_position = _object modelToWorldWorld [-(_finalPosition select 0), -(_finalPosition select 1), -(_finalPosition select 2)];
		}
		else {
			private _relativePosition = _position select 2;
			_position = _object modelToWorldWorld [-(_relativePosition select 0), -(_relativePosition select 1), -(_relativePosition select 2)];
		};
	};
}
else {
	if ((_position select 0) isEqualType []) then {
		_position = (_position select 0) vectorAdd (_position select 1);
	};
};

private "_primaryAxis";
private "_primaryAxisIndex";
private "_secondaryAxis";
private "_secondaryAxisIndex";
private "_tertiaryAxis";
private "_tertiaryAxisIndex";

{
	if (_x isEqualType []) then {
		_primaryAxis = _x select 0;
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

switch _type do {
	case "RECTANGLE": {
		private _secondaryAxisIteration = -_secondaryAxis;
		private _tertiaryAxisIteration = -_tertiaryAxis;

		for "_secondaryAxisIteration" from -_secondaryAxis to _secondaryAxis step _step do {
			for "_tertiaryAxisIteration" from -_tertiaryAxis to _tertiaryAxis step _step do {
				private _currentPositionStart = [0, 0, 0];
				private _currentPositionEnd = [0, 0, 0];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxis];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, -_primaryAxis];
				_currentPositionEnd set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, _tertiaryAxisIteration];

				if !(isNull _object) then {
					private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, objNull, objNull, _sort, -1, _lod1, _lod2, _returnUnique];
				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};
	
	case "OVAL": {
		private "_secondaryAxisIteration";
		private "_tertiaryAxisIteration";
		private "_primaryAxisIteration";
		private _phi = -90;
		private _theta = 0;
		
		for "_phi" from -90 to 90 step (_step * 20) do {
			for "_theta" from 0 to 360 step (_step * 20) do {
				_secondaryAxisIteration = _secondaryAxis * (cos _phi) * (cos _theta);
				_tertiaryAxisIteration = _tertiaryAxis * (cos _phi) * (sin _theta);
				_primaryAxisIteration = _primaryAxis * (sin _phi);
				private _currentPositionStart = [0, 0, 0];
				private _currentPositionEnd = [0, 0, 0];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxisIteration];
				_currentPositionEnd set [_secondaryAxisIndex, -_secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, -_tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, -_primaryAxisIteration];
				
				if !(isNull _object) then {
					private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, objNull, objNull, _sort, -1, _lod1, _lod2, _returnUnique];
				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};

	case "CYLINDER": {
		private "_secondaryAxisIteration";
		private "_tertiaryAxisIteration";
		private "_primaryAxisIteration";
		private _theta = 0;
		private _currentHeight = -_primaryAxis;
		
		for "_theta" from 0 to 360 step (_step * 20) do {
			for "_currentHeight" from -_primaryAxis to _primaryAxis step _step do {
				_secondaryAxisIteration = _secondaryAxis * (cos _theta);
				_tertiaryAxisIteration = _tertiaryAxis * (sin _theta);
				_primaryAxisIteration = -_currentHeight;
				private _currentPositionStart = [0, 0, 0];
				private _currentPositionEnd = [0, 0, 0];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionStart set [_primaryAxisIndex, _primaryAxisIteration];
				_currentPositionEnd set [_secondaryAxisIndex, -_secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, -_tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, _primaryAxisIteration];
				
				if !(isNull _object) then {
					private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, objNull, objNull, _sort, -1, _lod1, _lod2, _returnUnique];
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
		private _theta = 0;
		private _currentHeight = 0;
		private _currentRadiusOffset = 0;
		
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
					private _currentPositionStart = [0, 0, 0];
					private _currentPositionEnd = [0, 0, 0];
					_currentPositionStart set [_secondaryAxisIndex, 0];
					_currentPositionStart set [_tertiaryAxisIndex, 0];
					_currentPositionStart set [_primaryAxisIndex, 0];
					_currentPositionEnd set [_secondaryAxisIndex, _surfaceSecondary];
					_currentPositionEnd set [_tertiaryAxisIndex, _surfaceTertiary];
					_currentPositionEnd set [_primaryAxisIndex, _currentHeight];
					
					if !(isNull _object) then {
						private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
						_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
						private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
						_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
					}
					else {
						_currentPositionStart = _position vectorAdd _currentPositionStart;
						_currentPositionEnd = _position vectorAdd _currentPositionEnd;
					};

					_grids pushBack [_currentPositionStart, _currentPositionEnd, objNull, objNull, _sort, -1, _lod1, _lod2, _returnUnique];
					_intersectionCheckCount = _intersectionCheckCount + 1;
				};
			};
		};
	};
};

if _temporaryObject then {
	deleteVehicle _object;
};

if _verboseRays then {
	private _intersections = [];

	{
		_x set [5, 1];
		private _currentIntersection = lineIntersectsSurfaces _x;

		if (_currentIntersection isNotEqualTo []) then {
			_currentIntersection pushBack [_x select 0, _x select 1, (_x select 0) distance ((_currentIntersection select 0) select 0), (_x select 1) distance ((_currentIntersection select 0) select 0), true];
		}
		else {
			_currentIntersection pushBack [_x select 0, _x select 1, -1, -1, false];
		};

		_intersections pushBack _currentIntersection;
	} forEach _grids;

	if ((_ignoredObjects isNotEqualTo []) || (_ignoredTypes isNotEqualTo [])) then {
		private _invalidIntersections = [];

		{
			if !((_x select -1) select -1) then {
				continue;
			};
			
			private _intersectObject = (_x select 0) select 2;
			private _parentObject = (_x select 0) select 3;

			if ((_intersectObject in _ignoredObjects) || (_parentObject in _ignoredObjects)) then {
				_invalidIntersections pushBack _x;
				continue;
			};

			if ((isNull _intersectObject) && (isNull _parentObject) && ("TERRAIN" in _ignoredTypes)) then {
				_invalidIntersections pushBack _x;
				continue;
			};

			{
				if ((_intersectObject isKindOf _x) || (_parentObject isKindOf _x)) then {
					_invalidIntersections pushBack _x;
					break;
				};
			} forEach _ignoredTypes;
		} forEach _intersections;

		{
			private _currentIntersection = (_intersections select (_intersections find _x));
			(_currentIntersection select 1) set [-1, false];
			_currentIntersection = [_currentIntersection select 1]; 
		} forEach _invalidIntersections;
	};

	if (_draw isNotEqualTo []) then {
		private _draws = [];
		
		{
			if ((count (_intersections select _forEachIndex)) > 1) then {
				if (_draw select 0) then {
					_draws pushBack [_x select 0, ((_intersections select _forEachIndex) select 0) select 0];
				}
				else {
					_draws pushBack [ASLToAGL (_x select 0), ASLToAGL (((_intersections select _forEachIndex) select 0) select 0)];
				};
			}
			else {
				if (_draw select 0) then {
					_draws pushBack [_x select 0, _x select 1];
				}
				else {
					_draws pushBack [ASLToAGL (_x select 0), ASLToAGL (_x select 1)];
				};
			};
		} forEach _grids;

		if ((_draw select 2) > 0) then {
			private _drawHandler = [
				"MISSION",
				"Draw3D",
				[_draw, _draws],
				{
					_args params ["_draw", "_draws"];
					
					if (_draw select 0) then {
						{
							drawLaser [
								_x select 0,
								(_x select 0) vectorFromTo (_x select 1),
								(_draw select 1) select 0,
								(_draw select 1) select 1,
								(_draw select 1) select 2,
								(_draw select 1) select 3,
								(_x select 0) distance (_x select 1),
								(_draw select 1) select 4
							];
						} forEach _draws;
					}
					else {
						{
							drawLine3D [
								_x select 0, 
								_x select 1, 
								(_draw select 1) select 0, 
								(_draw select 1) select 1
							];
						} forEach _draws;
					};
				}
			] call KH_fnc_addEventHandler;

			[
				{
					params ["_drawHandler"];
					[_drawHandler] call KH_fnc_removeEventHandler;
				}, 
				[_drawHandler], 
				_draw select 2
			] call CBA_fnc_waitAndExecute;
		}
		else {
			if (_draw select 0) then {
				{
					drawLaser [
						_x select 0,
						(_x select 0) vectorFromTo (_x select 1),
						(_draw select 1) select 0,
						(_draw select 1) select 1,
						(_draw select 1) select 2,
						(_draw select 1) select 3,
						(_x select 0) distance (_x select 1),
						(_draw select 1) select 4
					];
				} forEach _draws;
			}
			else {
				{
					drawLine3D [
						_x select 0, 
						_x select 1, 
						(_draw select 1) select 0, 
						(_draw select 1) select 1
					];
				} forEach _draws;
			};
		};
	};

	[_intersections, _intersectionCheckCount, count (_intersections select {(_x select -1) select -1})];
}
else {
	private _intersections = (lineIntersectsSurfaces [_grids]) select {_x isNotEqualTo []};

	if ((_ignoredObjects isNotEqualTo []) || (_ignoredTypes isNotEqualTo [])) then {
		private _invalidIntersections = [];

		{
			private _intersectObject = (_x select 0) select 2;
			private _parentObject = (_x select 0) select 3;

			if ((_intersectObject in _ignoredObjects) || (_parentObject in _ignoredObjects)) then {
				_invalidIntersections pushBack _x;
				continue;
			};

			if ((isNull _intersectObject) && (isNull _parentObject) && ("TERRAIN" in _ignoredTypes)) then {
				_invalidIntersections pushBack _x;
				continue;
			};

			{
				if ((_intersectObject isKindOf _x) || (_parentObject isKindOf _x)) then {
					_invalidIntersections pushBack _x;
					break;
				};
			} forEach _ignoredTypes;
		} forEach _intersections;

		{
			_intersections deleteAt (_intersections find _x);
		} forEach _invalidIntersections;
	};

	if (_draw isNotEqualTo []) then {
		private _draws = [];
		
		{
			if (_draw select 0) then {
				_draws pushBack [_x select 0, _x select 1];
			}
			else {
				_draws pushBack [ASLToAGL (_x select 0), ASLToAGL (_x select 1)];
			};
		} forEach _grids;
		
		if ((_draw select 2) > 0) then {
			private _drawHandler = [
				"MISSION",
				"Draw3D",
				[_draw, _draws],
				{
					_args params ["_draw", "_draws"];
					
					if (_draw select 0) then {
						{
							drawLaser [
								_x select 0,
								(_x select 0) vectorFromTo (_x select 1),
								(_draw select 1) select 0,
								(_draw select 1) select 1,
								(_draw select 1) select 2,
								(_draw select 1) select 3,
								(_x select 0) distance (_x select 1),
								(_draw select 1) select 4
							];
						} forEach _draws;
					}
					else {
						{
							drawLine3D [
								_x select 0, 
								_x select 1, 
								(_draw select 1) select 0, 
								(_draw select 1) select 1
							];
						} forEach _draws;
					};
				}
			] call KH_fnc_addEventHandler;

			[
				{
					params ["_drawHandler"];
					[_drawHandler] call KH_fnc_removeEventHandler;
				}, 
				[_drawHandler], 
				_draw select 2
			] call CBA_fnc_waitAndExecute;
		}
		else {
			if (_draw select 0) then {
				{
					drawLaser [
						_x select 0,
						(_x select 0) vectorFromTo (_x select 1),
						(_draw select 1) select 0,
						(_draw select 1) select 1,
						(_draw select 1) select 2,
						(_draw select 1) select 3,
						(_x select 0) distance (_x select 1),
						(_draw select 1) select 4
					];
				} forEach _draws;
			}
			else {
				{
					drawLine3D [
						_x select 0, 
						_x select 1, 
						(_draw select 1) select 0, 
						(_draw select 1) select 1
					];
				} forEach _draws;
			};
		};
	};

	[_intersections, _intersectionCheckCount, count _intersections];
};