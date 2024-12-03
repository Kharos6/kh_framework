params ["_position", "_distances"];
private _positionAsl = [];

if (_position isEqualType objNull) then {
	_positionAsl = getPosASL _position;
}
else {
	_positionAsl = _position;
};

private _positionAgl = ASLToAGL _positionAsl;
private _dirX = (_distances select 0) min 5000;
private _dirY = (_distances select 1) min 5000;
private _dirZ = (_distances select 2) min 5000;
private _east = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, 0, 0], _entity]) select 0;
private _west = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, 0, 0], _entity]) select 0;
private _north = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, _dirY, 0], _entity]) select 0;
private _south = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, -_dirY, 0], _entity]) select 0;
private _above = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, 0, _dirZ], _entity]) select 0;
private _below = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, 0, -_dirZ], _entity]) select 0;
private _northEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, _dirY, 0], _entity]) select 0;
private _northWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, _dirY, 0], _entity]) select 0;
private _southEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, -_dirY, 0], _entity]) select 0;
private _southWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, -_dirY, 0], _entity]) select 0;
private _northAbove = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, _dirY, _dirZ], _entity]) select 0;
private _northBelow = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, _dirY, -_dirZ], _entity]) select 0;
private _southAbove = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, -_dirY, _dirZ], _entity]) select 0;
private _southBelow = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [0, -_dirY, -_dirZ], _entity]) select 0;
private _aboveEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, 0, _dirZ], _entity]) select 0;
private _aboveWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, 0, _dirZ], _entity]) select 0;
private _belowEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, 0, -_dirZ], _entity]) select 0;
private _belowWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, 0, -_dirZ], _entity]) select 0;
private _aboveNorthEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, _dirY, _dirZ], _entity]) select 0;
private _aboveNorthWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, _dirY, _dirZ], _entity]) select 0;
private _belowNorthEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, _dirY, -_dirZ], _entity]) select 0;
private _belowNorthWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, _dirY, -_dirZ], _entity]) select 0;
private _aboveSouthEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, -_dirY, _dirZ], _entity]) select 0;
private _aboveSouthWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, -_dirY, _dirZ], _entity]) select 0;
private _belowSouthEast = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [_dirX, -_dirY, -_dirZ], _entity]) select 0;
private _belowSouthWest = (lineIntersectsSurfaces [_positionAsl, _positionAsl vectorAdd [-_dirX, -_dirY, -_dirZ], _entity]) select 0;
private _closeObjects = [];

{
	_closeObjects pushBack ((lineIntersectsSurfaces [_positionAsl, getPosASL _x, _entity]) select 0);
} forEach ((nearestObjects [_ruinPosition, [], (_distances call BIS_fnc_arithmeticMean) + 30, false]) + (nearestTerrainObjects [_positionAgl, [], (_distances call BIS_fnc_arithmeticMean) + 30, false]));

private _nearestSurface = [999999, [0, 0, 0], objNull];
private _allObjects = _closeObjects append [
	_east, _west, _north, _south, _above, _below, 
	_northEast, _northWest, _southEast, _southWest, _northAbove, _northBelow, 
	_southAbove, _southBelow, _aboveEast, _aboveWest, _belowEast, _belowWest, 
	_aboveNorthEast, _aboveNorthWest, _belowNorthEast, _belowNorthWest, _aboveSouthEast, _aboveSouthWest, 
	_belowSouthEast, _belowSouthWest
];

{
	private _distance = _positionAgl distance (ASLToAGL (_x select 0));

	if (_distance <= (_nearestSurface select 0)) then {
		_nearestSurface = [_distance, _x select 0, _x select 2];
	};
} forEach _allObjects;

_nearestSurface;