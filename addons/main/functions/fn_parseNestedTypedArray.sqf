params [["_array", [], [[]]], ["_returnArray", [], [[]]]];

{
    if (_x isEqualType []) then {
        if ((count _x) isEqualTo 2) then {
            if (((_x select 0) isEqualType []) && ((_x select 1) isEqualType [])) then {
                _returnArray pushBack [];
                [_x, _returnArray] call KH_fnc_parseTypedArray;
            }
            else {
                _returnArray pushBack (_x call KH_fnc_parseValue);
            };
        }
        else {
            _returnArray pushBack [];
            [_x, _returnArray] call KH_fnc_parseTypedArray;
        };
    };
} forEach _array;

_returnArray;
