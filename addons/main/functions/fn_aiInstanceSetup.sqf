params [
    ["_name", "", [""]],
    ["_model", "", [""]],
    ["_markerSystemStart", "<|begin_of_text|><|start_header_id|>system<|end_header_id|>", ["", text ""]],
    ["_markerSystemEnd", "<|eot_id|>", ["", text ""]],
    ["_markerUserStart", "<|start_header_id|>user<|end_header_id|>", ["", text ""]],
    ["_markerUserEnd", "<|eot_id|>", ["", text ""]],
    ["_markerAssistantStart", "<|start_header_id|>assistant<|end_header_id|>", ["", text ""]],
    ["_markerAssistantEnd", "<|eot_id|>", ["", text ""]],
    ["_systemPrompt", "", ["", text "", {}]],
    ["_userPrompt", "", ["", text "", {}]],
    ["_contextSize", 32768, [0]],
    ["_maximumGeneratedTokens", 3072, [0]],
    ["_temperature", 0.3, [0]],
    ["_topK", 30, [0]],
    ["_topP", 0.9, [0]],
    ["_batchSize", 2048, [0]],
    ["_microBatchSize", 1024, [0]],
    ["_cpuThreads", 4, [0]],
    ["_cpuThreadsBatch", 6, [0]],
    ["_gpuLayers", 999, [0]],
    ["_flashAttention", true, [true]],
    ["_offloadKvCache", true, [true]],
    ["_responseProgressFunction", {}, [{}]],
    ["_responseFunction", {}, [{}]],
    ["_init", {}, [{}]],
    ["_immediateInference", false, [true]],
    ["_logGeneration", false, [true]]
];

if (_model isNotEqualTo "") then {
    _name setAiModel _model;
};

_name setAiParameters [_contextSize, _maximumGeneratedTokens, _temperature, _topK, _topP, _batchSize, _microBatchSize, _cpuThreads, _cpuThreadsBatch, _gpuLayers, _flashAttention, _offloadKvCache];

_name setAiMarkers [
    [[_markerSystemStart] joinString "", _markerSystemStart] select (_markerSystemStart isEqualType ""), 
    [[_markerSystemEnd] joinString "", _markerSystemEnd] select (_markerSystemEnd isEqualType ""), 
    [[_markerUserStart] joinString "", _markerUserStart] select (_markerUserStart isEqualType ""), 
    [[_markerUserEnd] joinString "", _markerUserEnd] select (_markerUserEnd isEqualType ""), 
    [[_markerAssistantStart] joinString "", _markerAssistantStart] select (_markerAssistantStart isEqualType ""), 
    [[_markerAssistantEnd] joinString "", _markerAssistantEnd] select (_markerAssistantEnd isEqualType "")
];

if ((_systemPrompt isNotEqualTo {}) && (_systemPrompt isNotEqualTo "") && (_systemPrompt isNotEqualTo (text ""))) then {
    switch (typeName _systemPrompt) do {
        case "STRING": {
            _name updateAiSystemPrompt _systemPrompt;
        };

        case "TEXT": {
            _name updateAiSystemPrompt ([_systemPrompt] joinString "");
        };

        case "CODE": {
            _name updateAiSystemPrompt ([_name] call _systemPrompt);
        };
    };
};

if ((_userPrompt isNotEqualTo {}) && (_userPrompt isNotEqualTo "") && (_userPrompt isNotEqualTo (text ""))) then {
    switch (typeName _userPrompt) do {
        case "STRING": {
            _name updateAiUserPrompt _userPrompt;
        };

        case "TEXT": {
            _name updateAiUserPrompt ([_userPrompt] joinString "");
        };

        case "CODE": {
            _name updateAiUserPrompt ([_name] call _userPrompt);
        };
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