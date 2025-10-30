KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            [
                [_logic, _units],
                {
                    params ["_logic", "_units"];
                    private _duration = parseNumber (_logic getVariable ["KH_ModuleAddActionDuration", "0"]);
                    private _conditionExist = _logic getVariable ["KH_ModuleAddActionConditionExist", "true"];
                    private _conditionShow = _logic getVariable ["KH_ModuleAddActionConditionShow", "true"];
                    private _conditionStart = _logic getVariable ["KH_ModuleAddActionConditionStart", "true"];
                    private _conditionProgress = _logic getVariable ["KH_ModuleAddActionConditionProgress", "true"];
                    private _conditionComplete = _logic getVariable ["KH_ModuleAddActionConditionComplete", "true"];
                    private _detectionShow = _logic getVariable ["KH_ModuleAddActionDetectionShow", "true"];
                    private _detectionProgress = _logic getVariable ["KH_ModuleAddActionDetectionProgress", "false"];
                    private _parent = _logic getVariable ["KH_ModuleAddActionParent", ""];
                    private _useAllPlayers = [false, true] select (_units isEqualTo []);

                    private _arguments = [
                        _logic getVariable ["KH_ModuleAddActionName", ""],
                        [],
                        [
                            compile (_logic getVariable ["KH_ModuleAddActionFunctionStart", ""]),
                            compile (_logic getVariable ["KH_ModuleAddActionFunctionProgress", ""]),
                            compile (_logic getVariable ["KH_ModuleAddActionFunctionInterrupt", ""]),
                            compile (_logic getVariable ["KH_ModuleAddActionFunctionCancel", ""]),
                            compile (_logic getVariable ["KH_ModuleAddActionFunctionComplete", ""])
                        ],
                        [
                            if (_conditionExist isEqualTo "true") then {
                                {};
                            }
                            else {
                                if (
                                    (((_conditionExist select 0) isEqualTo "'") && ((_conditionExist select ((count _conditionExist) - 1)) isEqualTo "'")) || 
                                    (((_conditionExist select 0) isEqualTo '"') && ((_conditionExist select ((count _conditionExist) - 1)) isEqualTo '"')) ||
                                    (((_conditionExist select 0) isEqualTo '[') && ((_conditionExist select ((count _conditionExist) - 1)) isEqualTo ']')) &&
                                    !(";" in _conditionExist)
                                ) then {
                                    if ("," in _conditionExist) then {
                                        parseSimpleArray _conditionExist;
                                    }
                                    else {
                                        _conditionExist;
                                    };
                                }
                                else {
                                    compile _conditionExist;
                                };
                            },
                            if (
                                (((_conditionShow select 0) isEqualTo "'") && ((_conditionShow select ((count _conditionShow) - 1)) isEqualTo "'")) || 
                                (((_conditionShow select 0) isEqualTo '"') && ((_conditionShow select ((count _conditionShow) - 1)) isEqualTo '"')) ||
                                (((_conditionShow select 0) isEqualTo '[') && ((_conditionShow select ((count _conditionShow) - 1)) isEqualTo ']')) &&
                                !(";" in _conditionShow)
                            ) then {
                                if ("," in _conditionShow) then {
                                    parseSimpleArray _conditionShow;
                                }
                                else {
                                    _conditionShow;
                                };
                            }
                            else {
                                compile _conditionShow;
                            },
                            if (
                                (((_conditionStart select 0) isEqualTo "'") && ((_conditionStart select ((count _conditionStart) - 1)) isEqualTo "'")) || 
                                (((_conditionStart select 0) isEqualTo '"') && ((_conditionStart select ((count _conditionStart) - 1)) isEqualTo '"')) ||
                                (((_conditionStart select 0) isEqualTo '[') && ((_conditionStart select ((count _conditionStart) - 1)) isEqualTo ']')) &&
                                !(";" in _conditionStart)
                            ) then {
                                if ("," in _conditionStart) then {
                                    parseSimpleArray _conditionStart;
                                }
                                else {
                                    _conditionStart;
                                };
                            }
                            else {
                                compile _conditionStart;
                            },
                            if (
                                (((_conditionProgress select 0) isEqualTo "'") && ((_conditionProgress select ((count _conditionProgress) - 1)) isEqualTo "'")) || 
                                (((_conditionProgress select 0) isEqualTo '"') && ((_conditionProgress select ((count _conditionProgress) - 1)) isEqualTo '"')) ||
                                (((_conditionProgress select 0) isEqualTo '[') && ((_conditionProgress select ((count _conditionProgress) - 1)) isEqualTo ']')) &&
                                !(";" in _conditionProgress)
                            ) then {
                                if ("," in _conditionProgress) then {
                                    parseSimpleArray _conditionProgress;
                                }
                                else {
                                    _conditionProgress;
                                };
                            }
                            else {
                                compile _conditionProgress;
                            },
                            if (
                                (((_conditionComplete select 0) isEqualTo "'") && ((_conditionComplete select ((count _conditionComplete) - 1)) isEqualTo "'")) || 
                                (((_conditionComplete select 0) isEqualTo '"') && ((_conditionComplete select ((count _conditionComplete) - 1)) isEqualTo '"')) ||
                                (((_conditionComplete select 0) isEqualTo '[') && ((_conditionComplete select ((count _conditionComplete) - 1)) isEqualTo ']')) &&
                                !(";" in _conditionComplete)
                            ) then {
                                if ("," in _conditionComplete) then {
                                    parseSimpleArray _conditionComplete;
                                }
                                else {
                                    _conditionComplete;
                                };
                            }
                            else {
                                compile _conditionComplete;
                            }
                        ],
                        _logic getVariable ["KH_ModuleAddActionRepeatable", false],
                        _logic getVariable ["KH_ModuleAddActionExclusive", true],
                        ["PLAYERS", _logic getVariable ["KH_ModuleAddActionJIP", true]],
                        [[_duration, true] select (_duration isEqualTo -1), _logic getVariable ["KH_ModuleAddActionProgressDisplay", "NONE"]],
                        parseNumber (_logic getVariable ["KH_ModuleAddActionDistance", "3"]),
                        _logic getVariable ["KH_ModuleAddActionShowImmediately", true],
                        _logic getVariable ["KH_ModuleAddActionAllowIncapacitated", false],
                        _logic getVariable ["KH_ModuleAddActionHideOnUse", false],
                        _logic getVariable ["KH_ModuleAddActionDrawHint", false],
                        [
                            _logic getVariable ["KH_ModuleAddActionDetectionType", "VIEW"],
                            switch _detectionShow do {
                                case "true": {
                                    true;
                                };

                                case "false": {
                                    false;
                                };

                                default {
                                    _detectionShow;
                                };
                            }, 
                            switch _detectionProgress do {
                                case "true": {
                                    true;
                                };

                                case "false": {
                                    false;
                                };

                                default {
                                    _detectionProgress;
                                };
                            }
                        ],
                        _logic getVariable ["KH_ModuleAddActionUserInput", ""],
                        _logic getVariable ["KH_ModuleAddActionIdentifier", ""],
                        switch _parent do {
                            case "true": {
                                [true, _logic getVariable ["KH_ModuleAddActionHandleParentActionRecovery", true]];
                            };

                            case "false": {
                                [false, _logic getVariable ["KH_ModuleAddActionHandleParentActionRecovery", true]];
                            };

                            default {
                                [missionNamespace getVariable [_parent, objNull], _logic getVariable ["KH_ModuleAddActionHandleParentActionRecovery", true]];
                            };
                        }
                    ];

                    if _useAllPlayers then {
                        _arguments insert [0, [[true, _logic getVariable ["KH_ModuleAddActionHandleObjectActionRecovery", true]]]];
                        _arguments call KH_fnc_addAction;
                    }
                    else {
                        {
                            _arguments insert [0, [[_x, _logic getVariable ["KH_ModuleAddActionHandleObjectActionRecovery", true]]]];
                            _currentArguments call KH_fnc_addAction;
                        } forEach _units;
                    };
                },
                true,
                {KH_var_playersLoaded;},
                false
            ] call KH_fnc_execute;
        };
    }
];

nil;