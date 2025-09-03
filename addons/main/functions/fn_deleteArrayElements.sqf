params [["_array", [], [[]]], ["_condition", {}, [{}]]];
private _deletions = [];
private _breaker = false;

{
    if _breaker then {
        break;
    };

    if ([_x] call _condition) then {
        _deletions pushBack _forEachIndex;
    };
} forEach _array;

_array deleteAt _deletions;