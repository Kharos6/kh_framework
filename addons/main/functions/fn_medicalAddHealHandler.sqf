params [["_unit", objNull, [objNull]], ["_function", {}, [{}]]];
private _healHandlers = _unit getVariable "KH_var_allHealHandlers";
private _healHandlerId = generateUid;
_unit setVariable [_healHandlerId, true];

if (isNil "_healHandlers") then {
    _unit setVariable ["KH_var_allHealHandlers", []];
};

_healHandlers pushBack (
    compile ([
        "if (missionNamespace getVariable '", _healHandlerId, "') then {
            call ", _function, ";
        }
        else {
            [
                [_unit, _forEachIndex],
                {
                    params ['_unit', '_index'];
                    (_unit getVariable 'KH_var_allHealHandlers') deleteAt _index;
                },
                true,
                '-1',
                false
            ] call KH_fnc_execute;
        };"
    ] joinString "")
);

[_unit, _healHandlerId, false];