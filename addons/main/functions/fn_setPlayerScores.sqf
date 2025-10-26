params [["_unit", objNull, [objNull]], ["_scores", [], [[]]]];
private _finalScores = [];
private _currentScores = getPlayerScores _unit;

{
    _finalScores pushBack (_x - (_currentScores select _forEachIndex)) 
} forEach _scores;

_unit addPlayerScores _finalScores;