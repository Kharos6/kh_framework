params ["_arguments", ["_function", "", [""]], ["_caller", 2, [0]], ["_unscheduled", true, [true]], ["_firstAttempt", true, [true]]];
_argsCallback params ["_storedFunction"];

if !(isNil "_storedFunction") exitWith {
    missionNamespace setVariable [_function, _storedFunction];

    if (!isServer && !_firstAttempt) then {
        missionNamespace setVariable [_function, _storedFunction, 2];
    };

    if _unscheduled then {
        if (isNil "_arguments") then {
            call _storedFunction;
        }
        else {
            _arguments call _storedFunction;
        };
    }
    else {
        if (isNil "_arguments") then {
            [] spawn _storedFunction;
        }
        else {
            _arguments spawn _storedFunction;
        };
    };
};

if (!_firstAttempt || (_caller isEqualTo 2) || (_caller isEqualTo clientOwner)) exitWith {};

[
    [_arguments, _function, _caller, _unscheduled, false],
    "KH_fnc_processRemoteSerializedFunction",
    _caller,
    true,
    [
        "CALLBACK",
        [_function],
        "KH_fnc_retrieveSerializedFunction"
    ]
] call KH_fnc_execute;