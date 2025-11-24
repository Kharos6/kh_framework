params [["_model", "", [""]], ["_provider", "dml", [""]], ["_threads", 4, [0]], ["_noiseScale", 0.667, [0]], ["_noiseScaleW", 0.8, [0]], ["_lengthScale", 1, [0]], ["_functionGenerated", {}, [{}]], ["_functionFinished", {}, [{}]]];
_model ttsLoadModel [_provider, _threads, _noiseScale, _noiseScaleW, _lengthScale];
KH_var_ttsGeneratedHandlerFunction = _functionGenerated;
KH_var_ttsFinishedHandlerFunction = _functionFinished;

if (isNil "KH_var_ttsHandler") then {
    KH_var_ttsHandler = true;
    ["KH_eve_ttsGenerated", {call KH_var_ttsGeneratedHandlerFunction;}] call CBA_fnc_addEventHandler;
    ["KH_eve_ttsFinished", {call KH_var_ttsFinishedHandlerFunction;}] call CBA_fnc_addEventHandler;
};