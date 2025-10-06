params [["_array", [], [[], createHashMap]], ["_function", {}, [{}]], ["_layer", 0, [0]], ["_returnArray", [], [[]]], ["_breaker", "", [""]]];

if (_breaker isEqualTo "") then {
    _breaker = generateUid;
    missionNamespace setVariable [_breaker, false];
};

private _hashmap = if (_array isEqualType createHashMap) then {
    _array = toArray _array;
    true;
}
else {
    false;
};

{
    if (_breaker isEqualTo true) then {
        missionNamespace setVariable [_breaker, true];
        break;
    };

    if (missionNamespace getVariable _breaker) then {
        break;
    };

    if (_x isEqualType []) then {
        _returnArray pushBack [];
        [_x, _function, _layer + 1, _returnArray select -1, _breaker] call KH_fnc_nestedArrayOperation;
    }
    else {
        _returnArray pushBack (call _function);
    };
} forEach _array;

if _hashmap then {
    createHashMapFromArray _returnArray;
}
else {
    _returnArray;
};