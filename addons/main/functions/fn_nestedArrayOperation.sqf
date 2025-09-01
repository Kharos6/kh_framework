params [["_array", [], [[], createHashMap]], "_arguments", ["_function", {}, [{}]], ["_layer", 0, [0]], ["_evaluateKeyValueArrays", false, [true]], ["_returnArray", [], [[]]], ["_breaker", "", [""]]];

if (_breaker isEqualTo "") then {
    _breaker = call KH_fnc_generateUid;
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
        if _evaluateKeyValueArrays then {
            if ((count _x) isEqualTo 2) then {
                if (((_x select 0) isEqualType []) && ((_x select 1) isEqualType [])) then {
                    _returnArray pushBack [];
                    [_x, _arguments, _function, _layer + 1, _evaluateKeyValueArrays, _returnArray select -1, _breaker] call KH_fnc_nestedArrayOperation;
                }
                else {
                    _returnArray pushBack (_arguments call _function);
                };
            }
            else {
                _returnArray pushBack [];
                [_x, _arguments, _function, _layer + 1, _evaluateKeyValueArrays, _returnArray select -1, _breaker] call KH_fnc_nestedArrayOperation;
            };
        }
        else {
            _returnArray pushBack [];
            [_x, _arguments, _function, _layer + 1, _evaluateKeyValueArrays, _returnArray select -1, _breaker] call KH_fnc_nestedArrayOperation;
        };
    }
    else {
        _returnArray pushBack (_arguments call _function);
    };
} forEach _array;

if _hashmap then {
    createHashMapFromArray _returnArray;
}
else {
    _returnArray;
};