params ["_position", "_offset", ["_type", "BOX"], ["_step", 0.1], ["_draw", []], ["_ignored1", objNull], ["_ignored2", objNull], ["_sort", false], ["_maxResults", 1], ["_lod1", "VIEW"], ["_lod2", "GEOM"], ["_returnUnique", true]];
private _object = objNull;

if ((_position select 0) isEqualType objNull) then {
	_object = _position select 0;
	private _positionVector = _position select 1;
	_position = [-(_positionVector select 0), -(_positionVector select 1), -(_positionVector select 2)];
	_position = _object modelToWorldWorld _position;
};

private _primaryAxis = 999999;
private _primaryAxisIndex = 999999;
private _secondaryAxis = 999999;
private _secondaryAxisIndex = 999999;
private _tertiaryAxis = 999999;
private _tertiaryAxisIndex = 999999;
private _draws = [];
private _grids = [];

{
	if (_x isEqualType []) then {
		_primaryAxis = _x select 0;
		_primaryAxisIndex = _forEachIndex;
	}
	else {
		if (_secondaryAxis == 999999) then {
			_secondaryAxis = abs _x;
			_secondaryAxisIndex = _forEachIndex;
		}
		else {
			_tertiaryAxis = abs _x;
			_tertiaryAxisIndex = _forEachIndex;
		};
	};
} forEach _offset;

private _secondaryAxisIteration = _secondaryAxis;
private _tertiaryAxisIteration = _tertiaryAxis;
private _currentPositionStart = [0, 0, 0];
private _currentPositionEnd = [0, 0, 0];
private _intersectionCheckCount = 0;

switch _type do {
	case "BOX": {
		for "_secondaryAxisIteration" from -_secondaryAxis to _secondaryAxis step _step do {
			for "_tertiaryAxisIteration" from -_tertiaryAxis to _tertiaryAxis step _step do {
				_currentPositionStart set [_primaryAxisIndex, _primaryAxis];
				_currentPositionStart set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionStart set [_tertiaryAxisIndex, _tertiaryAxisIteration];
				_currentPositionEnd set [_primaryAxisIndex, -_primaryAxis];
				_currentPositionEnd set [_secondaryAxisIndex, _secondaryAxisIteration];
				_currentPositionEnd set [_tertiaryAxisIndex, _tertiaryAxisIteration];

				if !(isNull _object) then {
					private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
					private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
					_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
					_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
				}
				else {
					_currentPositionStart = _position vectorAdd _currentPositionStart;
					_currentPositionEnd = _position vectorAdd _currentPositionEnd;
				};

				_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResults, _lod1, _lod2, _returnUnique];

				if (_draw isNotEqualTo []) then {
					_draws pushBack [ASLToAGL _currentPositionStart, ASLToAGL _currentPositionEnd];
				};

				_intersectionCheckCount = _intersectionCheckCount + 1;
			};
		};
	};
	
	case "SPHERE": {
		private _radius = _secondaryAxis;
		private _phiStep = _step * 20;
		private _thetaStep = _step * 20;
		private _phi = -90;
		private _theta = 0;
		private _radiusOffset = 0;
		
		for "_phi" from -90 to 90 step _phiStep do {
			for "_theta" from 0 to 360 step _thetaStep do {
				for "_radiusOffset" from 0 to _radius step (_step * 2) do {					
					_currentPositionStart = [0, 0, 0];
					_currentPositionEnd set [0, _radiusOffset * (cos _phi) * (cos _theta)];
					_currentPositionEnd set [1, _radiusOffset * (cos _phi) * (sin _theta)];
					_currentPositionEnd set [2, _radiusOffset * (sin _phi)];

					if !(isNull _object) then {
						private _relativeOffsetStart = _object modelToWorldWorld _currentPositionStart;
						private _relativeOffsetEnd = _object modelToWorldWorld _currentPositionEnd;
						_currentPositionStart = _relativeOffsetStart vectorAdd (_relativeOffsetStart vectorDiff _position);
						_currentPositionEnd = _relativeOffsetEnd vectorAdd (_relativeOffsetEnd vectorDiff _position);
					}
					else {
						_currentPositionStart = _position vectorAdd _currentPositionStart;
						_currentPositionEnd = _position vectorAdd _currentPositionEnd;
					};

					_grids pushBack [_currentPositionStart, _currentPositionEnd, _ignored1, _ignored2, _sort, _maxResults, _lod1, _lod2, _returnUnique];

					if (_draw isNotEqualTo []) then {
						_draws pushBack [ASLToAGL _currentPositionStart, ASLToAGL _currentPositionEnd];
					};

					_intersectionCheckCount = _intersectionCheckCount + 1;
				};
			};
		};
	};
};

private _intersections = lineIntersectsSurfaces [_grids];

if (_draw isNotEqualTo []) then {
	private _drawHandler = [
		"MISSION",
		"Draw3D",
		[_draw, _draws],
		{
			_args params ["_draw", "_draws"];

			{
				drawLine3D [
					_x select 0, 
					_x select 1, 
					_draw select 0, 
					_draw select 1
				];
			} forEach _draws;
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
};

private _intersectionPercentage = if (_intersections isEqualTo []) then {
	0;
}
else {
	(_intersectionCheckCount / (count _intersections)) * 100;
};

[_intersections, _intersectionPercentage];