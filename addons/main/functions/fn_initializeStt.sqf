params [["_model", "", [""]], ["_threads", 2, [0]], ["_function", {}, [{}]]];
_model sttLoadModel [_threads];
KH_var_sttHandlerFunction = _function;

if (isNil "KH_var_sttHandler") then {
    KH_var_sttHandler = true;
    ["KH_eve_sttTranscription", {call KH_var_sttHandlerFunction;}] call CBA_fnc_addEventHandler;
};