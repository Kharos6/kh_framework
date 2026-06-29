params [["_target", objNull, [objNull, grpNull]], ["_persistentEventId", "", [""]], ["_persistentExecutionId", "", [""]], ["_initialId", "", [""]]];

if (local _target) then {
    if !(_target getVariable [_initialId, false]) then {
        (_target getVariable _persistentEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled"];
        _arguments callSerializedFunction [_function, _caller, _unscheduled];
    };
};

[
    ["ENTITY", _target, "LOCAL"],
    "Local",
    [_persistentExecutionId, _persistentEventId],
    {
        params ["_entity", "_local"];
        _args params ["_persistentExecutionId", "_persistentEventId"];

        if (_entity getVariable _persistentExecutionId) then {
            (_entity getVariable _persistentEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled"];
            
            if _local then {
                _arguments callSerializedFunction [_function, _caller, _unscheduled];
            }
            else {
                _sendoffArguments callSerializedFunction [_sendoffFunction, _caller, _unscheduled];
            };
        };
    }
] call KH_fnc_addEventHandler;