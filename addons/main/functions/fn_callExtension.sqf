params [["_extension", "", [""]], ["_arguments", nil, [[]]], ["_function", "", [""]], ["_resultParsing", false, [true, ""]]];

(_extension callExtension (
    if (isNil "_arguments") then {
        _function;
    }
    else {
        [_function, _arguments];
    }
)) params ["_result", "_returnCode", "_errorCode"];

if ((_returnCode isEqualTo 1) || (_errorCode isEqualTo 1)) exitWith {
    [
        "EXTENSION EXECUTION FAILURE:",
        [
            ["EXTENSION = ", _extension] joinString "",
            ["ARGUMENTS = ", _arguments] joinString "", 
            ["FUNCTION = ", _function] joinString "", 
            ["RESULT = ", _result] joinString "",
            ["RETURN CODE = ", _returnCode] joinString "",
            ["ERROR CODE = ", _errorCode] joinString ""
        ],
        true,
        true
    ] call KH_fnc_log;

    _result;
};

if (_resultParsing isEqualTo false) exitWith {
    _result;
};

if (_resultParsing isEqualTo true) then {
    _result = parseSimpleArray _result;

    if ((count _result) isEqualTo 1) then {
        _result select 0;
    }
    else {
        [_result select 0, _result select 1] call KH_fnc_parseValue;
    };
}
else {
    [_resultParsing, _result] call KH_fnc_parseValue;
};