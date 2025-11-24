params [["_model", "", [""]], ["_provider", "dml", [""]], ["_threads", 4, [0]], ["_function", {}, [{}]]];
_model sttLoadModel [_provider, _threads];
KH_var_sttHandlerFunction = _function;

if (isNil "KH_var_sttHandler") then {
    KH_var_sttHandler = true;
    ["KH_eve_sttTranscription", {call KH_var_sttHandlerFunction;}] call CBA_fnc_addEventHandler;
};