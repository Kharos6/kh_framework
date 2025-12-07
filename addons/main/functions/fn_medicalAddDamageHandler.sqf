params [["_unit", objNull, [objNull]], ["_function", {}, [{}]]];
private _damageHandlers = _unit getVariable "KH_var_allDamageHandlers";
private _damageHandlerId = generateUid;
_unit setVariable [_damageHandlerId, true];

if (isNil "_damageHandlers") then {
    _unit setVariable ["KH_var_allDamageHandlers", []];
};

_damageHandlers pushBack [
    "if (missionNamespace getVariable '", _damageHandlerId, "') then {
        call ", _function, ";
    }
    else {
        [
            [_unit, _forEachIndex],
            {
                params ['_unit', '_index'];
                (_unit getVariable 'KH_var_allDamageHandlers') deleteAt _index;
            },
            true,
            '-1',
            false
        ] call KH_fnc_execute;
    };"
] joinString "";

[_unit, _damageHandlerId, false];