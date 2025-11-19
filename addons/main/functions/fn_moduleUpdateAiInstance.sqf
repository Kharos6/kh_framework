KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            private _owner = _logic getVariable ["KH_ModuleUpdateAIInstanceOwner", "SERVER"];
            private _name = _logic getVariable ["KH_ModuleUpdateAIInstanceName", ""];
            private _systemPrompt = _logic getVariable ["KH_ModuleUpdateAIInstanceSystemPrompt", ""];
            private _userPrompt = _logic getVariable ["KH_ModuleUpdateAIInstanceUserPrompt", ""];
            
            if (_owner isEqualTo "SERVER") then {
                _name updateAiSystemPrompt _systemPrompt;
                _name updateAiUserPrompt _userPrompt;
            }
            else {
                [
                    [_owner, _name, _systemPrompt, _userPrompt]
                    {
                        _this deleteAt 0;

                        [
                            _this,
                            {
                                params ["_name", "_systemPrompt", "_userPrompt"];
                                _name updateAiSystemPrompt _systemPrompt;
                                _name updateAiUserPrompt _userPrompt;
                            },
                            missionNamespace getVariable _owner,
                            true,
                            false
                        ] call KH_fnc_execute;
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

            if (_logic getVariable ["KH_ModuleUpdateAIInstanceActivateOnce", true]) then {
                deleteVehicle _logic;
            };
        };
    }
];

nil;