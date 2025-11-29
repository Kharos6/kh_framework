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
            private _masterPrompt = compile (_logic getVariable ["KH_ModuleAIInstanceSetupMasterPrompt", ""]);
            private _userPrompt = compile (_logic getVariable ["KH_ModuleAIInstanceSetupUserPrompt", ""]);
            private _contextSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupContextSize", "32768"]);
            private _maximumGeneratedTokens = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMaximumGeneratedTokens", "3072"]);
            private _temperature = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTemperature", "0.3"]);
            private _topK = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTopK", "30"]);
            private _topP = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTopP", "0.9"]);
            private _minP = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMinP", "0.05"]);
            private _typicalP = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupTypicalP", "1"]);
            private _repeatPenalty = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupRepeatPenalty", "1.1"]);
            private _repetitionCheckTokenCount = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupRepetitionCheckTokenCount", "64"]);
            private _presencePenalty = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupPresencePenalty", "0"]);
            private _frequencyPenalty = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupFrequencyPenalty", "0"]);
            private _mirostat = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMirostat", "DISABLED"]);
            private _mirostatTau = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMirostatTau", "5"]);
            private _mirostatEta = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMirostatETA", "0.1"]);
            private _seed = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupSeed", "-1"]);
            private _batchSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupUserBatchSize", "2048"]);
            private _microBatchSize = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMicroBatchSize", "1024"]);
            private _cpuThreads = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupCPUThreads", "4"]);
            private _cpuThreadsBatch = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupCPUThreadsBatch", "6"]);
            private _gpuLayers = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupGPULayers", "999"]);
            private _flashAttention = _logic getVariable ["KH_ModuleAIInstanceSetupFlashAttention", true];
            private _offloadKvCache = _logic getVariable ["KH_ModuleAIInstanceSetupOffloadKVCache", true];
            private _mainGpu = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupMainGPU", "0"]);
            private _tensorSplit = parseSimpleArray (["[", _logic getVariable ["KH_ModuleAIInstanceSetupTensorSplit", ""], "]"] joinString "");
            private _splitMode = parseNumber (_logic getVariable ["KH_ModuleAIInstanceSetupSplitMode", "DISABLED"]);
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
                    call _systemPrompt,
                    call _masterPrompt,
                    call _userPrompt, 
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
                    _splitMode,
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
                        _masterPrompt,
                        _userPrompt, 
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
                        _splitMode,
                        _responseProgressFunction,
                        _responseFunction,
                        _init,
                        _immediateInference,
                        _logGeneration
                    ],
                    {
                        params ["_owner"];
                        _this set [9, call (_this select 9)];
                        _this set [10, call (_this select 10)];
                        _this set [11, call (_this select 11)];
                        _this deleteAt 0;
                        [_this, "KH_fnc_aiInstanceSetup", [missionNamespace getVariable _owner, KH_var_allPlayerUidMachines get _owner] select ((_owner select [0, 1]) isNotEqualTo 0), true, false] call KH_fnc_execute;
                    },
                    true,
                    {
                        params ["_owner"];
                        
                        if ((_owner select [0, 1]) isNotEqualTo 0) then {
                            !(isNil {KH_var_allPlayerUidMachines get _owner;});
                        }
                        else {
                            if !(missionNamespace isNil _owner) then {
                                private _unit = missionNamespace getVariable _owner;
                                (!(local _unit) && !(isNull _unit));
                            };
                        };
                    },
                    false
                ] call KH_fnc_execute;
            };
        };
    }
];

nil;