params [["_unit", objNull, [objNull]]];
if ((getNumber (configFile >> (getText ((configOf _unit) >> "moves")) >> "states" >> (animationState _unit) >> "kh_traversal")) isEqualTo 1) exitWith {};
private _currentPosition = getPosASLVisual _unit;
private _unitPositionHeight = _currentPosition select 2;
private _bounds = (2 boundingBoxReal _unit) select 1;
private _unitWidth = (_bounds select 0) * 0.5;
private _unitLength = (_bounds select 1) * 0.5;
private _unitHeight = _bounds select 2;

private _obstacleIntersections = ([
    _currentPosition vectorAdd [0, 0, _unitHeight + 5.1], 
    _unit,
    [_unitWidth, _unitLength, str -(_unitHeight + 5)],
    "RECTANGLE",
    0.25,
    [_unit, "TERRAIN"] + (attachedObjects _unit),
    true, 
    1, 
    "GEOM", 
    "ROADWAY",
    true,
    []
] call KH_fnc_raycast3d) select 0;

private _maximumHeight = if (_obstacleIntersections isNotEqualTo []) then {
    private _highestPoint = 10;

    {
        private _currentHeight = ((_x select 0) select 2) - _unitPositionHeight;

        if (_currentHeight < _highestPoint) then {
            _highestPoint = _currentHeight;
        };
    } forEach _obstacleIntersections;

    _highestPoint;
}
else {
    10;
};

if ((_maximumHeight - _unitHeight) < 0.25) exitWith {
    [_unit, [[0, "ACTION_PLAY_NOW", "KH_TraversalJump", true], [0, "ACTION_PLAY", "default", true]], false, false] call KH_fnc_setAnimation;
    false;
};

private _climbIntersections = ([
    _unit modelToWorldVisualWorld [0, _unitLength * 1.75, _maximumHeight], 
    _unit,
    [_unitWidth, _unitLength, str _maximumHeight],
    "RECTANGLE",
    0.25,
    [_unit, "TERRAIN"] + (attachedObjects _unit),
    false, 
    -1, 
    "GEOM", 
    "ROADWAY",
    true,
    []
] call KH_fnc_raycast3d) select 0;

if (_climbIntersections isEqualTo []) exitWith {
    [_unit, [[0, "ACTION_PLAY_NOW", "KH_TraversalJump", true], [0, "ACTION_PLAY", "default", true]], false, false] call KH_fnc_setAnimation;
    false;
};

private _floorIndex = -1;
private _floorPositions = [];

{
    private _intersectionPosition = _x select 0;

    if (
        (((_intersectionPosition select 2) - _unitPositionHeight) >= 0.25) &&
        (((_intersectionPosition select 2) - _unitPositionHeight) > ((((_floorPositions param [[_floorIndex, 0] select (_floorIndex isEqualTo -1), []]) param [2, 0]) + _unitHeight) - _unitPositionHeight))
       ) 
    then {
        _floorPositions pushBack _intersectionPosition;
        _floorIndex = _floorIndex + 1;
    };
} forEach _climbIntersections;

if (_floorPositions isEqualTo []) exitWith {
    [_unit, [[0, "ACTION_PLAY_NOW", "KH_TraversalJump", true], [0, "ACTION_PLAY", "default", true]], false, false] call KH_fnc_setAnimation;
    false;
};

private _chosenTraversalTarget = if (isPlayer _unit) then {
    private _traversalTargets = createHashMap;

    {
        _traversalTargets set [([objNull, ASLToAGL _x] call KH_fnc_getPositionScreenRatio) select 1, _x];
    } forEach _floorPositions;

    private _finalTarget = _traversalTargets get (selectMax (keys _traversalTargets));

    if (isNil "_finalTarget") then {
        selectRandom _floorPositions;
    }
    else {
        _finalTarget;
    };
}
else {
    selectRandom _floorPositions;
};

private _height = (_chosenTraversalTarget select 2) - _unitPositionHeight;

private _vault = ((([
    _unit modelToWorldVisualWorld [0, _unitLength * 2.75, _height], 
    _unit,
    [_unitWidth, _unitLength, str _unitHeight],
    "RECTANGLE",
    0.25,
    [_unit, "TERRAIN"] + (attachedObjects _unit),
    true, 
    1, 
    "GEOM", 
    "ROADWAY",
    true,
    []
] call KH_fnc_raycast3d) select 0) param [0, []]) isEqualTo [];

_unit setVariable ["KH_var_traversalTarget", [_chosenTraversalTarget select 0, (_unit modelToWorldVisualWorld [0, _unitLength * 2.75, 0]) select 1, _chosenTraversalTarget select 2]];

[
    _unit, 
    [
        [
            0,
            "ACTION_PLAY_NOW", 
            switch true do {
                case ((_height >= 0.25) && (_height < 0.75)): {
                    ["KH_TraversalClimb0_5m", "KH_TraversalVault0_5m"] select _vault;
                };

                case ((_height >= 0.75) && (_height < 1.25)): {
                    ["KH_TraversalClimb1m", "KH_TraversalVault1m"] select _vault;
                };

                case ((_height >= 1.25) && (_height < 1.75)): {
                    ["KH_TraversalClimb1_5m", "KH_TraversalVault1_5m"] select _vault;
                };

                case ((_height >= 1.75) && (_height < 2.25)): {
                    ["KH_TraversalClimb2m", "KH_TraversalVault2m"] select _vault;
                };

                case ((_height >= 0.25) && (_height < 0.75)): {
                    ["KH_TraversalClimb0_5m", "KH_TraversalVault0_5m"] select _vault;
                };

                case ((_height >= 0.75) && (_height < 1.25)): {
                    ["KH_TraversalClimb1m", "KH_TraversalVault1m"] select _vault;
                };

                case ((_height >= 1.25) && (_height < 1.75)): {
                    ["KH_TraversalClimb1_5m", "KH_TraversalVault1_5m"] select _vault;
                };

                case ((_height >= 1.75) && (_height < 2.25)): {
                    ["KH_TraversalClimb2m", "KH_TraversalVault2m"] select _vault;
                };

                case ((_height >= 2.25) && (_height < 2.75)): {
                    ["KH_TraversalClimb2_5m", "KH_TraversalVault2_5m"] select _vault;
                };

                case ((_height >= 2.75) && (_height < 3.25)): {
                    ["KH_TraversalClimb3m", "KH_TraversalVault3m"] select _vault;
                };

                case ((_height >= 3.25) && (_height < 3.75)): {
                    ["KH_TraversalClimb3_5m", "KH_TraversalVault3_5m"] select _vault;
                };

                case ((_height >= 3.75) && (_height < 4.25)): {
                    ["KH_TraversalClimb4m", "KH_TraversalVault4m"] select _vault;
                };

                case ((_height >= 4.25) && (_height < 4.75)): {
                    ["KH_TraversalClimb4_5m", "KH_TraversalVault4_5m"] select _vault;
                };

                case ((_height >= 4.75) && (_height < 5.25)): {
                    ["KH_TraversalClimb5m", "KH_TraversalVault5m"] select _vault;
                };

                case ((_height >= 5.25) && (_height < 5.75)): {
                    ["KH_TraversalClimb5_5m", "KH_TraversalVault5_5m"] select _vault;
                };

                case ((_height >= 5.75) && (_height < 6.25)): {
                    ["KH_TraversalClimb6m", "KH_TraversalVault6m"] select _vault;
                };

                case ((_height >= 6.25) && (_height < 6.75)): {
                    ["KH_TraversalClimb6_5m", "KH_TraversalVault6_5m"] select _vault;
                };

                case ((_height >= 6.75) && (_height < 7.25)): {
                    ["KH_TraversalClimb7m", "KH_TraversalVault7m"] select _vault;
                };

                case ((_height >= 7.25) && (_height < 7.75)): {
                    ["KH_TraversalClimb7_5m", "KH_TraversalVault7_5m"] select _vault;
                };

                case ((_height >= 7.75) && (_height < 8.25)): {
                    ["KH_TraversalClimb8m", "KH_TraversalVault8m"] select _vault;
                };

                case ((_height >= 8.25) && (_height < 8.75)): {
                    ["KH_TraversalClimb8_5m", "KH_TraversalVault8_5m"] select _vault;
                };

                case ((_height >= 8.75) && (_height < 9.25)): {
                    ["KH_TraversalClimb9m", "KH_TraversalVault9m"] select _vault;
                };

                case ((_height >= 9.25) && (_height < 9.75)): {
                    ["KH_TraversalClimb9_5m", "KH_TraversalVault9_5m"] select _vault;
                };

                case ((_height >= 9.75) && (_height < 10.25)): {
                    ["KH_TraversalClimb10m", "KH_TraversalVault10m"] select _vault;
                };
                
                default {
                    "default";
                };
            }, 
            true
        ], 
        [
            0,
            "ACTION_PLAY", 
            "default", 
            true
        ]
    ], 
    false,
    false
] call KH_fnc_setAnimation;

true;