params [
    ["_name", "", [""]],
    ["_model", "", [""]],
    ["_markerSystemStart", "", [""]],
    ["_markerSystemEnd", "", [""]],
    ["_markerUserStart", "", [""]],
    ["_markerUserEnd", "", [""]],
    ["_markerAssistantStart", "", [""]],
    ["_markerAssistantEnd", "", [""]],
    ["_systemPrompt", "", ["", text ""]],
    ["_masterPrompt", "", ["", text ""]],
    ["_userPrompt", "", ["", text ""]],
    ["_contextSize", 32768, [0]],
    ["_maximumGeneratedTokens", 3072, [0]],
    ["_temperature", 0.3, [0]],
    ["_topK", 30, [0]],
    ["_topP", 0.9, [0]],
    ["_minP", 0.05, [0]],
    ["_typicalP", 1, [0]],
    ["_repeatPenalty", 1.1, [0]],
    ["_repetitionCheckTokenCount", 64, [0]],
    ["_presencePenalty", 0, [0]],
    ["_frequencyPenalty", 0, [0]],
    ["_mirostat", 0, [0]],
    ["_mirostatTau", 5, [0]],
    ["_mirostatEta", 0.1, [0]],
    ["_seed", -1, [0]],
    ["_batchSize", 2048, [0]],
    ["_microBatchSize", 1024, [0]],
    ["_cpuThreads", 4, [0]],
    ["_cpuThreadsBatch", 6, [0]],
    ["_gpuLayers", 999, [0]],
    ["_flashAttention", true, [true]],
    ["_offloadKvCache", true, [true]],
    ["_mainGpu", 0, [0]],
    ["_tensorSplit", [], [[]]],
    ["_splitMode", 0, [0]],
    ["_responseProgressFunction", {}, [{}]],
    ["_responseFunction", {}, [{}]],
    ["_init", {}, [{}]],
    ["_immediateInference", false, [true]],
    ["_logGeneration", false, [true]]
];

if (_model isNotEqualTo "") then {
    _name setAiModel _model;
};

_name setAiParameters [
    _contextSize, 
    _maximumGeneratedTokens, 
    _temperature, 
    _topK, 
    _topP, 
    _minP,
    _typicalP,
    _repeatPenalty,
    _repetitionCheckTokenCount,
    _presencePenalty,
    _frequencyPenalty,
    _mirostat,
    _mirostatTau,
    _mirostatEta,
    _seed,
    _batchSize, 
    _microBatchSize, 
    _cpuThreads, 
    _cpuThreadsBatch, 
    _gpuLayers, 
    _flashAttention, 
    _offloadKvCache, 
    _mainGpu, 
    _tensorSplit,
    _splitMode
];

_name setAiMarkers [
    [[_markerSystemStart] joinString "", _markerSystemStart] select (_markerSystemStart isEqualType ""), 
    [[_markerSystemEnd] joinString "", _markerSystemEnd] select (_markerSystemEnd isEqualType ""), 
    [[_markerUserStart] joinString "", _markerUserStart] select (_markerUserStart isEqualType ""), 
    [[_markerUserEnd] joinString "", _markerUserEnd] select (_markerUserEnd isEqualType ""), 
    [[_markerAssistantStart] joinString "", _markerAssistantStart] select (_markerAssistantStart isEqualType ""), 
    [[_markerAssistantEnd] joinString "", _markerAssistantEnd] select (_markerAssistantEnd isEqualType "")
];

if ((_systemPrompt isNotEqualTo "") && (_systemPrompt isNotEqualTo (text ""))) then {
    if (_systemPrompt isEqualType "") then {
        _name updateAiSystemPrompt _systemPrompt;
    }
    else {
        _name updateAiSystemPrompt ([_systemPrompt] joinString "");
    };
};

if ((_masterPrompt isNotEqualTo "") && (_masterPrompt isNotEqualTo (text ""))) then {
    if (_masterPrompt isEqualType "") then {
        _name updateAiMasterPrompt _masterPrompt;
    }
    else {
        _name updateAiMasterPrompt ([_masterPrompt] joinString "");
    };
};

if ((_userPrompt isNotEqualTo "") && (_userPrompt isNotEqualTo (text ""))) then {
    if (_userPrompt isEqualType "") then {
        _name updateAiUserPrompt _userPrompt;
    }
    else {
        _name updateAiUserPrompt ([_userPrompt] joinString "");
    };
};

if (_responseProgressFunction isNotEqualTo {}) then {
    ["KH_eve_aiResponseProgress", _responseProgressFunction] call CBA_fnc_addEventHandler;
};

if (_responseFunction isNotEqualTo {}) then {
    ["KH_eve_aiResponse", _responseFunction] call CBA_fnc_addEventHandler;
};

if (_init isNotEqualTo {}) then {
    ["KH_eve_aiInitialized", _init] call CBA_fnc_addEventHandler;
};

if _logGeneration then {
    _name logAiGeneration true;
};

if _immediateInference then {
    [
        "CBA",
        "KH_eve_aiInitialized",
        [_name],
        {
            params ["_name"];
            triggerAiInference _name;
            [_handlerId] call KH_fnc_removeHandler;
        }
    ] call KH_fnc_addEventHandler;
};

initializeAi _name;