params [
    ["_name", "", [""]], 
    ["_systemPrompt", "", ["", text "", {}]],
    ["_masterPrompt", "", ["", text "", {}]],
    ["_userPrompt", "", ["", text "", {}]], 
    ["_responseProgressFunction", {}, [{}]],
    ["_responseFunction", {}, [{}]],
    ["_triggerInference", true, [true]], 
    ["_abortGeneration", true, [true]], 
    ["_resetContext", false, [true]], 
    ["_logGeneration", false, [true]], 
    ["_stop", false, [true]]
];

private _aiInstanceHandlerId = ["KH_var_aiInstanceHandler_", _name] joinString "";
private _aiInstanceArgumentsId = ["KH_var_aiInstanceArguments_", _name] joinString "";
private _aiInstanceTriggerId = ["KH_var_aiInstanceTrigger_", _name] joinString "";
private _aiInstanceResponseProgressHandlerId = ["KH_var_aiInstanceResponseProgressHandler_", _name] joinString "";
private _aiInstanceResponseHandlerId = ["KH_var_aiInstanceResponseHandler_", _name] joinString "";
missionNamespace setVariable [_aiInstanceArgumentsId, [_systemPrompt, _userPrompt, _responseProgressFunction, _responseFunction, _abortGeneration, _resetContext, _logGeneration, _stop]];
missionNamespace setVariable [_aiInstanceTriggerId, _triggerInference];

if (missionNamespace isNil _aiInstanceHandlerId) then {
    missionNamespace setVariable [_aiInstanceHandlerId, false];

    [
        [_name, _aiInstanceHandlerId, _aiInstanceArgumentsId, _aiInstanceTriggerId, _aiInstanceResponseProgressHandlerId, _aiInstanceResponseHandlerId],
        {
            params ["_name", "_aiInstanceHandlerId", "_aiInstanceArgumentsId", "_aiInstanceTriggerId", "_aiInstanceResponseProgressHandlerId", "_aiInstanceResponseHandlerId"];
            (missionNamespace getVariable _aiInstanceArgumentsId) params ["_systemPrompt", "_userPrompt", "_responseProgressFunction", "_responseFunction", "_abortGeneration", "_resetContext", "_logGeneration", "_stop"];
            if !(isAiActive _name) exitWith {};
            
            if _stop exitWith {
                stopAi _name;
                missionNamespace setVariable [_aiInstanceHandlerId, nil];
                [_handlerId] call KH_fnc_removeHandler;
            };

            if (missionNamespace getVariable _aiInstanceHandlerId) exitWith {};
            missionNamespace setVariable [_aiInstanceHandlerId, true];

            if (isAiGenerating _name) then {
                if _abortGeneration then {
                    abortAiGeneration _name;
                };
            };

            if _resetContext then {
                resetAiContext _name;
            };
            
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

            if ((_masterPrompt isNotEqualTo {}) && (_masterPrompt isNotEqualTo "") && (_masterPrompt isNotEqualTo (text ""))) then {
                switch (typeName _masterPrompt) do {
                    case "STRING": {
                        _name updateAiMasterPrompt _masterPrompt;
                    };

                    case "TEXT": {
                        _name updateAiMasterPrompt ([_masterPrompt] joinString "");
                    };

                    case "CODE": {
                        _name updateAiMasterPrompt ([_name] call _masterPrompt);
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

            _name logAiGeneration _logGeneration;

            if !(missionNamespace isNil _aiInstanceResponseProgressHandlerId) then {
                [missionNamespace getVariable _aiInstanceResponseProgressHandlerId] call KH_fnc_removeHandler;
            };

            if !(missionNamespace isNil _aiInstanceResponseHandlerId) then {
                [missionNamespace getVariable _aiInstanceResponseHandlerId] call KH_fnc_removeHandler;
            };

            private _responseProgressHandler = [
                "CBA",
                "KH_eve_aiResponseProgress",
                [_name, _aiInstanceArgumentsId],
                {
                    params ["_name", "_response"];
                    _args params ["_thisName", "_aiInstanceArgumentsId"];

                    if (_name isEqualTo _thisName) then {
                        private _responseProgressFunction = (missionNamespace getVariable _aiInstanceArgumentsId) select 2;

                        if (_responseProgressFunction isNotEqualTo {}) then {
                            [_name, _response] call _responseProgressFunction;
                        };
                    };
                }
            ] call KH_fnc_addEventHandler;

            missionNamespace setVariable [_aiInstanceResponseProgressHandlerId, _responseProgressHandler];

            missionNamespace setVariable [
                _aiInstanceResponseHandlerId,
                [
                    "CBA",
                    "KH_eve_aiResponse",
                    [_name, _aiInstanceHandlerId, _aiInstanceArgumentsId, _responseProgressHandler],
                    {
                        params ["_name", "_response"];
                        _args params ["_thisName", "_aiInstanceHandlerId", "_aiInstanceArgumentsId", "_responseProgressHandler"];

                        if (_name isEqualTo _thisName) then {
                            private _responseFunction = (missionNamespace getVariable _aiInstanceArgumentsId) select 3;

                            if (_responseFunction isNotEqualTo {}) then {
                                [_name, _response] call _responseFunction;
                            };

                            [_responseProgressHandler] call KH_fnc_removeHandler;
                            [_handlerId] call KH_fnc_removeHandler;
                        };
                    }
                ] call KH_fnc_addEventHandler
            ];

            if (missionNamespace getVariable _aiInstanceTriggerId) then {
                if (isAiGenerating _name) then {
                    abortAiGeneration _name;
                };

                triggerAiInference _name;
            };
        },
        true,
        0,
        false
    ] call KH_fnc_execute;
}
else {
    missionNamespace setVariable [_aiInstanceHandlerId, false];
};