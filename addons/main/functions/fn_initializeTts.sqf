params [["_model", "", [""]], ["_threads", 2, [0]], ["_noiseScale", 0.667, [0]], ["_noiseScaleW", 0.8, [0]], ["_lengthScale", 1, [0]], ["_functionGenerated", {}, [{}]], ["_functionFinished", {}, [{}]]];
_model ttsLoadModel [_threads, _noiseScale, _noiseScaleW, _lengthScale];
KH_var_ttsGeneratedHandlerFunction = _functionGenerated;
KH_var_ttsFinishedHandlerFunction = _functionFinished;

if (isNil "KH_var_ttsHandler") then {
    KH_var_ttsHandler = true;
    ["CBA", "KH_eve_ttsGenerated", [], {call KH_var_ttsGeneratedHandlerFunction;}] call KH_fnc_addEventHandler;
    ["CBA", "KH_eve_ttsFinished", [], {call KH_var_ttsFinishedHandlerFunction;}] call KH_fnc_addEventHandler;
};