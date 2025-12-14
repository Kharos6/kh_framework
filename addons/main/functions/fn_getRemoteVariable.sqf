params [
    ["_namespace", missionNamespace, [true, objNull, grpNull, locationNull, displayNull, controlNull, missionNamespace]], 
    ["_name", "", ["", []]], 
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]]
];

if (_target isEqualType teamMemberNull) then {
    _target = agent _target;
};

private _variableId = [hashValue _namespace, hashValue _name, hashValue _target] joinString "";
private _variableHandlerId = ["KH_var_variableHandler_", hashValue _namespace, hashValue _name, hashValue _target] joinString "";

if !(missionNamespace isNil _variableId) exitWith {
    private _value = missionNamespace getVariable _variableId;
    missionNamespace setVariable [_variableId, nil];
    missionNamespace setVariable [_variableHandlerId, nil];
    _value;
};

if (missionNamespace isNil _variableHandlerId) then {
    missionNamespace setVariable [_variableHandlerId, true];

    [
        [_variableId],
        {
            params ["_variableId"];
            missionNamespace setVariable [_variableId, _argsCallback];
        },
        _target,
        true,
        [
            "CALLBACK",
            [_namespace, _name],
            {
                params ["_namespace", "_name"];

                if (_name isEqualType "") then {
                    [_namespace getVariable _name];
                }
                else {
                    private _values = [];

                    {
                        _values pushBack (_namespace getVariable _x);
                    } forEach _name;

                    _values;
                };
            }
        ]
    ] call KH_fnc_execute;
};

nil;