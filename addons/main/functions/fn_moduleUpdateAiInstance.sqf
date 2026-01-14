isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        private _owner = _logic getVariable ["KH_ModuleUpdateAIInstanceOwner", ""];
        private _name = _logic getVariable ["KH_ModuleUpdateAIInstanceName", ""];
        private _systemPrompt = compile (_logic getVariable ["KH_ModuleUpdateAIInstanceSystemPrompt", ""]);
        private _masterPrompt = compile (_logic getVariable ["KH_ModuleUpdateAIInstanceMasterPrompt", ""]);
        private _userPrompt = compile (_logic getVariable ["KH_ModuleUpdateAIInstanceUserPrompt", ""]);
        private _responseProgressFunction = compile (_logic getVariable ["KH_ModuleUpdateAIInstanceResponseProgressFunction", ""]);
        private _responseFunction = compile (_logic getVariable ["KH_ModuleUpdateAIInstanceResponseFunction", ""]);
        private _triggerInference = _logic getVariable ["KH_ModuleUpdateAIInstanceTriggerInference", true];
        private _abortGeneration = _logic getVariable ["KH_ModuleUpdateAIInstanceAbortGeneration", true];
        private _resetContext = _logic getVariable ["KH_ModuleUpdateAIInstanceResetContext", false];
        private _logGeneration = _logic getVariable ["KH_ModuleUpdateAIInstanceLogGeneration", false];
        private _stop = _logic getVariable ["KH_ModuleUpdateAIInstanceStop", false];
        
        if (_owner isEqualTo "") then {
            [_name, call _systemPrompt, call _masterPrompt, call _userPrompt, _responseProgressFunction, _responseFunction, _triggerInference, _abortGeneration, _resetContext, _logGeneration, _stop] call KH_fnc_updateAiInstance;
        }
        else {
            [
                [_owner, _name, _systemPrompt, _masterPrompt, _userPrompt, _responseProgressFunction, _responseFunction, _triggerInference, _abortGeneration, _resetContext, _logGeneration, _stop],
                {
                    _this set [2, call (_this select 2)];
                    _this set [3, call (_this select 3)];
                    _this set [4, call (_this select 4)];
                    _this deleteAt 0;
                    [_this, "KH_fnc_updateAiInstance", missionNamespace getVariable _owner, true, false] call KH_fnc_execute;
                },
                true,
                {
                    params ["_owner"];
                    
                    if ((parseNumber (_owner select [0, 1])) isNotEqualTo 0) then {
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

        if (_logic getVariable ["KH_ModuleUpdateAIInstanceActivateOnce", true]) then {
            deleteVehicle _logic;
        };
    };
}