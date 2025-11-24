KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            private _owner = _logic getVariable ["KH_ModuleAIInstanceSetupOwner", "SERVER"];
            private _name = _logic getVariable ["KH_ModuleAIInstanceSetupName", ""];
            private _model = _logic getVariable ["KH_ModuleAIInstanceSetupModel", ""];
            private _markerSystemStart = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerSystemStart", "<|begin_of_text|><|start_header_id|>system<|end_header_id|>"];
            private _markerSystemEnd = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerSystemEnd", "<|eot_id|>"];
            private _markerUserStart = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerUserStart", "<|start_header_id|>user<|end_header_id|>"];
            private _markerUserEnd = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerUserEnd", "<|eot_id|>"];
            private _markerAssistantStart = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerAssistantStart", "<|start_header_id|>assistant<|end_header_id|>"];
            private _markerAssistantEnd = _logic getVariable ["KH_ModuleAIInstanceSetupMarkerAssistantEnd", "<|eot_id|>"];
            private _systemPrompt = compile (_logic getVariable ["KH_ModuleAIInstanceSetupSystemPrompt", ""]);
            private _userPrompt = compile (_logic getVariable ["KH_ModuleAIInstanceSetupUserPrompt", ""]);
            private _contextSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupContextSize", "32768"]);
            private _maximumGeneratedTokens = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMaximumGeneratedTokens", "3072"]);
            private _temperature = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTemperature", "0.3"]);
            private _topK = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTopK", "30"]);
            private _topP = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTopP", "0.9"]);
            private _batchSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupUserBatchSize", "2048"]);
            private _microBatchSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMicroBatchSize", "1024"]);
            private _cpuThreads = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupCPUThreads", "4"]);
            private _cpuThreadsBatch = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupCPUThreadsBatch", "6"]);
            private _gpuLayers = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupGPULayers", "999"]);
            private _flashAttention = _logic getVariable ["KH_ModuleAIInstanceSetupFlashAttention", true];
            private _offloadKvCache = _logic getVariable ["KH_ModuleAIInstanceSetupOffloadKVCache", true];
            private _responseProgressFunction = compile (_logic getVariable ["KH_ModuleAIInstanceSetupResponseProgressFunction", ""]);
            private _responseFunction = compile (_logic getVariable ["KH_ModuleAIInstanceSetupResponseFunction", ""]);
            private _init = compile (_logic getVariable ["KH_ModuleAIInstanceSetupInit", ""]);
            private _immediateInference = _logic getVariable ["KH_ModuleAIInstanceSetupImmediateInference", false];
            private _logGeneration = _logic getVariable ["KH_ModuleAIInstanceSetupLogGeneration", false];
            
            if (_owner isEqualTo "SERVER") then {
                [
                    _name, 
                    _model,
                    _markerSystemStart, 
                    _markerSystemEnd, 
                    _markerUserStart, 
                    _markerUserEnd, 
                    _markerAssistantStart, 
                    _markerAssistantEnd,
                    _systemPrompt, 
                    _userPrompt, 
                    _contextSize, 
                    _maximumGeneratedTokens, 
                    _temperature, 
                    _topK, 
                    _topP, 
                    _batchSize, 
                    _microBatchSize, 
                    _cpuThreads, 
                    _cpuThreadsBatch,
                    _gpuLayers, 
                    _flashAttention, 
                    _offloadKvCache,
                    _responseProgressFunction,
                    _responseFunction,
                    _init,
                    _immediateInference,
                    _logGeneration
                ] call KH_fnc_aiInstanceSetup;                
            }
            else {
                [
                    [
                        _owner,
                        _name, 
                        _model,
                        _markerSystemStart, 
                        _markerSystemEnd, 
                        _markerUserStart, 
                        _markerUserEnd, 
                        _markerAssistantStart, 
                        _markerAssistantEnd,
                        _systemPrompt, 
                        _userPrompt, 
                        _contextSize, 
                        _maximumGeneratedTokens, 
                        _temperature, 
                        _topK, 
                        _topP, 
                        _batchSize, 
                        _microBatchSize, 
                        _cpuThreads, 
                        _cpuThreadsBatch,
                        _gpuLayers, 
                        _flashAttention, 
                        _offloadKvCache,
                        _responseProgressFunction,
                        _responseFunction,
                        _init,
                        _immediateInference,
                        _logGeneration
                    ],
                    {
                        _this deleteAt 0;
                        [_this, "KH_fnc_aiInstanceSetup", missionNamespace getVariable _owner, true, false] call KH_fnc_execute;
                    },
                    true,
                    {
                        params ["_owner"];
                        
                        if !(missionNamespace isNil _owner) then {
                            private _unit = missionNamespace getVariable _owner;
                            (!(local _unit) && !(isNull _unit));
                        };
                    },
                    false
                ] call KH_fnc_execute;
            };
        };
    }
];

nil;