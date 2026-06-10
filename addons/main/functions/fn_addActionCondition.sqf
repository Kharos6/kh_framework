params [["_action", "", [""]], "_arguments", ["_condition", {true;}, [{}]]];
private _id = generateUid;
private _argumentsId = generateUid;
private _conditionId = generateUid;
private _handlerConditionId = generateUid;
missionNamespace setVariable [_id, true];
missionNamespace setVariable [_argumentsId, _arguments];
missionNamespace setVariable [_conditionId, _condition];

private _unitActions = [KH_var_playerUnit];

KH_var_playerUnit addAction [ 
    "",
    {nil;}, 
    nil,
    1.5, 
    false, 
    true, 
    _action, 
    [
        "private _handlerId = [missionNamespace, '", _id, "', clientOwner];
        if (missionNamespace getVariable '", _id, "') then {
            (missionNamespace getVariable '", _argumentsId, "') call (missionNamespace getVariable '", _conditionId, "');
        }
        else {
            _originalTarget removeAction _actionId;
            false;
        };"
    ] joinString "", 
    0, 
    false, 
    "", 
    "" 
];

[
    "CBA",
    "KH_eve_playerControlledUnitChanged",
    [_action, _condition, _id, _unitActions, _argumentsId, _conditionId],
    {
        private _unit = param [3];
        _args params ["_action", "_condition", "_id", "_unitActions", "_argumentsId", "_conditionId"];

        if !(missionNamespace getVariable _id) exitWith {
            [_handlerId] call KH_fnc_removeHandler;
        };

        if !(_unit in _unitActions) then {
            _unitActions pushBack _unit; 

            _unit addAction [ 
                "",
                {nil;}, 
                nil,
                1.5, 
                false, 
                true, 
                _action, 
                [
                    "private _handlerId = [missionNamespace, '", _id, "', clientOwner];
                    if (missionNamespace getVariable '", _id, "') then {
                        (missionNamespace getVariable '", _argumentsId, "') call (missionNamespace getVariable '", _conditionId, "');
                    }
                    else {
                        _originalTarget removeAction _actionId;
                        false;
                    };"
                ] joinString "", 
                0, 
                false, 
                "", 
                "" 
            ];
        };
    }
] call KH_fnc_addEventHandler;

[missionNamespace, _id, clientOwner];