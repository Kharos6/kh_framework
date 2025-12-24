params [["_unit", objNull, [objNull]], ["_scores", [], [[]]]];
if (((count _scores) isNotEqualTo 5) || !(isPlayer _unit)) exitWith {};

if (time > 1) then {
    private _currentScores = getPlayerScores _unit;
    private _finalScores = [];

    {
        _finalScores pushBack (_x - (_currentScores select _forEachIndex)); 
    } forEach _scores;

    if ((count _finalScores) isEqualTo 5) then {
        _unit addPlayerScores _finalScores;
    };
}
else {
    [[_unit, _scores], "KH_fnc_setPlayerScores", true, {time > 1;}, false] call KH_fnc_execute;
};