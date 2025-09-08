params [["_array", [], [[]]], ["_condition", {}, [[], {}]]];
private _deletions = [];

if (_condition isEqualType []) then {
    {
        private _index = _array find _x;

        if (_index isNotEqualTo -1) then {
            _deletions pushBack _index;
        };
    } forEach _condition;
}
else {
    private _breaker = false;

    {
        if _breaker then {
            break;
        };

        if (call _condition) then {
            _deletions pushBack _forEachIndex;
        };
    } forEach _array;
};

if (_deletions isNotEqualTo []) then {
    _array deleteAt _deletions;
    true;
}
else {
    false;
};