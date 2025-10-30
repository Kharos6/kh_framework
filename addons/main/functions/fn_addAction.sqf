params [
    ["_object", true, [true, objNull, []]],
    ["_name", "", ["", []]],
    ["_arguments", []],
    ["_function", {}, [[], {}]],
    ["_condition", {true;}, ["", [], {}]],
    ["_repeatable", false, [true]],
    ["_exclusive", true, [true]],
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
    ["_duration", false, [true, 0, []]],
    ["_distance", 3, [0]],
    ["_showImmediately", true, [true]],
    ["_allowIncapacitated", false, [true]],
    ["_hideOnUse", false, [true]],
    ["_drawHint", false, [true, []]],
    ["_detection", true, [true, "", []]],
    ["_userInput", "", [""]],
    ["_identifier", "", [""]],
    ["_parent", true, [true, objNull, []]]
];

private "_progressDisplay";
private "_handleObjectActionRecovery";
private "_handleParentActionRecovery";
private "_jip";

if (_object isEqualType []) then {
    _handleObjectActionRecovery = _object param [1, true, [true]];
    _object = _object param [0, true, [true, objNull]];
}
else {
    _handleObjectActionRecovery = true;
};

private _visiblePerPlayer = false;

private _continue = if (_object isEqualType true) then {
    _visiblePerPlayer = true;
    true;
}
else {
    !(isNull _object);
};

if !_continue exitWith {
    [];
};

if (_target isEqualType []) then {
    _jip = _target param [1, true, [true]];
    _target = _target param [0, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]];
}
else {
    _jip = true;
};

if (_duration isEqualType []) then {
    _progressDisplay = _duration param [1, "NONE", [""]];
    _duration = _duration param [0, false, [true, 0]];
}
else {
    _progressDisplay = "NONE";
};

if (_parent isEqualType []) then {
    _handleParentActionRecovery = _parent param [1, true, [true]];
    _parent = _parent param [0, nil, [objNull]];
}
else {
    _handleParentActionRecovery = true;
};

if (_target isEqualTo "GLOBAL") then {
    _target = "PLAYERS";
}
else {
    if (_target isEqualType []) then {
        if ("PLAYERS" in _target) then {
            _target = "PLAYERS";
        };
    };
};

if (_duration isEqualType true) then {
    if !_duration then {
        _duration = 0;
    };
}
else {
    _duration = _duration max 0;
};

if _visiblePerPlayer then {
    _drawHint = [];
}
else {
    if (_drawHint isEqualTo false) then {
        _drawHint = [];
    }
    else {
        if (_drawHint isEqualTo true) then {
            _drawHint = [
                "ICON",
                [
                    "\x\kh\addons\main\ui\helper_arrow_up.paa",
                    [profileNamespace getVariable ["GUI_BCG_RGB_R", 0.13], profileNamespace getVariable ["GUI_BCG_RGB_G", 0.54], profileNamespace getVariable ["GUI_BCG_RGB_B", 0.21], 0.9],
                    if (_detection isEqualType "") then {
                        _object modelToWorldVisual (_object selectionPosition _detection);
                    }
                    else {
                        unitAimPositionVisual _object;
                    },
                    0.25,
                    0.25,
                    1,
                    if (_name isEqualType "") then {
                        _name;
                    }
                    else {
                        _name select 0;
                    },
                    true,
                    0.02,
                    "EtelkaMonospacePro",
                    "center",
                    false,
                    0,
                    0
                ],
                true,
                [],
                {}
            ];
        }
        else {
            _drawHint = ["ICON", _drawHint, true, [], {}];
        };
    };
};

private _parsedIdentifier = if (_identifier isEqualTo "") then {
    generateUid;
}
else {
    _identifier;
};

private _parsedActionId = ["KH_var_actionId", _parsedIdentifier] joinString "_";
private _actionExistenceId = ["KH_var_actionExistenceId", _parsedIdentifier] joinString "_";
private _actionSafetyId = ["KH_var_actionSafetyId", _parsedIdentifier] joinString "_";
private _actionStorageId = ["KH_var_actionStorageId", _parsedIdentifier] joinString "_";
private _completionId = ["KH_var_actionCompletionId", _parsedIdentifier] joinString "_";
private _conditionShowId = ["KH_var_actionShowId", _parsedIdentifier] joinString "_";
private _targetId = ["KH_var_actionTargetId", _parsedIdentifier] joinString "_";
private _firstCall = false;

if (isNil {missionNamespace getVariable _parsedActionId;}) then {
    _firstCall = true;
    missionNamespace setVariable [_parsedActionId, true, true];
    missionNamespace setVariable [_actionExistenceId, true, true];
    missionNamespace setVariable [_conditionShowId, true, true];
    missionNamespace setVariable [_actionStorageId, [], true];
    missionNamespace setVariable [_completionId, false, true];
};

if !(missionNamespace getVariable _actionExistenceId) exitWith {
    [];
};

private _parsedConditionExist = if (_firstCall && (_condition isEqualType [])) then {
    _condition params ["_conditionExist", {}, ["", [], {}]];

    if (_conditionExist isNotEqualTo {}) then {
        switch (typeName _conditionExist) do {
            case "STRING": {
                if !(_object isEqualType true) then {
                    compile ([
                        "(missionNamespace getVariable '", _objectId, "') getVariable ['", _conditionExist, "', false];"
                    ] joinString "");
                }
                else {
                    compile (["missionNamespace getVariable ['", _conditionExist, "', false];"] joinString "");
                };
            };

            case "ARRAY": {
                private _conditionsId = generateUid;
                missionNamespace setVariable [_conditionsId, _conditionExist];
                
                if !(_object isEqualType true) then {
                    compile ([
                        "private _result = true;

                        {
                            if (_x isEqualType '') then {
                                if !((missionNamespace getVariable '", _objectId, "') getVariable [_x, false]) then {
                                    _result = false;
                                    break;
                                };
                            }
                            else {
                                private _value = (missionNamespace getVariable '", _objectId, "') getVariable (_x select 0);
                                
                                if (isNil '_value') then {
                                    _result = false;
                                    break;
                                };

                                if (_value isNotEqualTo (_x select 1)) then {
                                    _result = false;
                                    break;
                                };
                            };
                        } forEach (missionNamespace getVariable '", _conditionsId, "');
                        
                        _result;"
                    ] joinString "");
                }
                else {
                    compile ([
                        "private _result = true;

                        {
                            if (_x isEqualType '') then {
                                if !(missionNamespace getVariable [_x, false]) then {
                                    _result = false;
                                    break;
                                };
                            }
                            else {
                                private _value = missionNamespace getVariable (_x select 0);
                                
                                if (isNil '_value') then {
                                    _result = false;
                                    break;
                                };

                                if (_value isNotEqualTo (_x select 1)) then {
                                    _result = false;
                                    break;
                                };
                            };
                        } forEach (missionNamespace getVariable '", _conditionsId, "');
                        
                        _result;"
                    ] joinString "");
                };
            };

            case "CODE": {
                _conditionExist;
            };
        };
    };
};

_continue = if !(isNil "_parsedConditionExist") then {
    (_arguments call _parsedConditionExist);
}
else {
    true;
};

if !_continue exitWith {
    [];
};

private _actionHandler = [
    [
        _object, 
        _name, 
        _arguments, 
        _function, 
        _condition, 
        _repeatable, 
        _exclusive,
        _target,
        _duration, 
        _distance,
        _showImmediately,
        _allowIncapacitated,
        _hideOnUse,
        _drawHint, 
        _detection, 
        _userInput, 
        _progressDisplay,
        _parsedIdentifier,
        _parent,
        _handleObjectActionRecovery,
        _handleParentActionRecovery,
        _visiblePerPlayer,
        _actionExistenceId,
        _actionSafetyId,
        _actionStorageId,
        _completionId,
        _conditionShowId,
        _targetId
    ],
    {
        params [
            "_object", 
            "_name", 
            "_arguments", 
            "_function", 
            "_condition",
            "_repeatable", 
            "_exclusive",
            "_target",
            "_duration", 
            "_distance",
            "_showImmediately",
            "_allowIncapacitated",
            "_hideOnUse",
            "_drawHint",
            "_detection",
            "_userInput",
            "_progressDisplay",
            "_parsedIdentifier",
            "_parent",
            "_handleObjectActionRecovery",
            "_handleParentActionRecovery",
            "_visiblePerPlayer",
            "_actionExistenceId",
            "_actionSafetyId",
            "_actionStorageId",
            "_completionId",
            "_conditionShowId",
            "_targetId"
        ];

        if (!hasInterface || (_parent isEqualTo objNull) || (_object isEqualTo objNull) || !(missionNamespace getVariable _actionExistenceId) || (missionNamespace getVariable _completionId)) exitWith {};

        if (isNull KH_var_playerUnit) exitWith {
            [
                [
                    [_object, _handleObjectActionRecovery],
                    _name, 
                    _arguments, 
                    _function, 
                    _condition, 
                    _repeatable, 
                    _exclusive,
                    [true, false],
                    [_duration, _progressDisplay], 
                    _distance,
                    _showImmediately,
                    _allowIncapacitated,
                    _hideOnUse,
                    _drawHint, 
                    _detection, 
                    _userInput, 
                    _parsedIdentifier,
                    [_parent, _handleParentActionRecovery]
                ],
                "KH_fnc_addAction",
                true,
                {!(isNull KH_var_playerUnit)},
                false
            ] call KH_fnc_execute;
        };
        
        private _parsedObject = [_object, [KH_var_playerUnit, player] select (_object isEqualTo true)] select _visiblePerPlayer;
        private _playerOnly = false;

        private _parsedParent = if (_parent isEqualType true) then {
            if _parent then {
                _playerOnly = true;
                player;
            }
            else {
                KH_var_playerUnit;
            };
        }
        else {
            _parent;
        };

        private _allowedActivationId = generateUid;
        missionNamespace setVariable [_allowedActivationId, true];
        private _objectId = generateUid;
        missionNamespace setVariable [_objectId, _parsedObject];
        private "_menuName";
        private "_windowBackgroundName";
        private "_windowForegroundName";
        private "_progressMenuName";
        private "_progressWindowBackgroundName";
        private "_progressWindowForegroundName";
        private "_conditionShow";
        private "_conditionStart";
        private "_conditionProgress";
        private "_conditionComplete";
        private "_functionStart";
        private "_functionProgress";
        private "_functionInterrupt";
        private "_functionCancel";
        private "_functionComplete";
        private "_detectionType";
        private "_initialDetection";
        private "_progressDetection";

        if (_name isEqualType "") then {
            _menuName = _name;
            _windowBackgroundName = _name;
            _windowForegroundName = _name;
            _progressMenuName = _name;
            _progressWindowBackgroundName = _name;
            _progressWindowForegroundName = _name;
        }
        else {
            _menuName = _name param [0, "", [""]];
            _windowBackgroundName = _name param [1, "", [""]];
            _windowForegroundName = _name param [2, "", [""]];
            _progressMenuName = _name param [3, "", [""]];
            _progressWindowBackgroundName = _name param [4, "", [""]];
            _progressWindowForegroundName = _name param [5, "", [""]];
        };

        private _conditionParser = {
            params [["_condition", {true;}, ["", [], {}]]];

            switch (typeName _condition) do {
                case "STRING": {
                    compile ([
                        "(missionNamespace getVariable '", _objectId, "') getVariable ['", _condition, "', false];"
                    ] joinString "");
                };

                case "ARRAY": {
                    private _conditionsId = generateUid;
                    missionNamespace setVariable [_conditionsId, _condition];
                    
                    compile ([
                        "private _result = true;

                        {
                            if (_x isEqualType '') then {
                                if !((missionNamespace getVariable '", _objectId, "') getVariable [_x, false]) then {
                                    _result = false;
                                    break;
                                };
                            }
                            else {
                                private _value = (missionNamespace getVariable '", _objectId, "') getVariable (_x select 0);
                                
                                if (isNil '_value') then {
                                    _result = false;
                                    break;
                                };

                                if (_value isNotEqualTo (_x select 1)) then {
                                    _result = false;
                                    break;
                                };
                            };
                        } forEach (missionNamespace getVariable '", _conditionsId, "');
                        
                        _result;"
                    ] joinString "");
                };

                case "CODE": {
                    _condition;
                };
            };
        };

        if (_condition isEqualType {}) then {
            _conditionShow = _condition;
            _conditionStart = _condition;
            _conditionProgress = _condition;
            _conditionComplete = _condition;
        }
        else {
            if ((count _condition) <= 1) then {
                _conditionShow = {true;};
                _conditionStart = {true;};
                _conditionProgress = {true;};
                _conditionComplete = {true;};
            }
            else {
                _condition = _condition select [1];
                _conditionShow = [_condition param [0, {true;}, ["", [], {}]]] call _conditionParser;
                _conditionStart = [_condition param [1, {true;}, ["", [], {}]]] call _conditionParser;
                _conditionProgress = [_condition param [2, {true;}, ["", [], {}]]] call _conditionParser;
                _conditionComplete = [_condition param [3, {true;}, ["", [], {}]]] call _conditionParser;
            };
        };

        if (_function isEqualType {}) then {
            _functionStart = {};
            _functionProgress = {};
            _functionInterrupt = {};
            _functionCancel = {};
            _functionComplete = _function;
        }
        else {
            _functionStart = _function param [0, _functionStart, [{}]];
            _functionProgress = _function param [1, _functionProgress, [{}]];
            _functionInterrupt = _function param [2, _functionInterrupt, [{}]];
            _functionCancel = _function param [3, _functionCancel, [{}]];
            _functionComplete = _function param [4, {}, [{}]];
        };
        
        if (_detection isEqualType []) then {
            _detectionType = _detection param [0, "VIEW", [""]];
            _initialDetection = _detection param [1, true, [true, ""]];
            _progressDetection = _detection param [2, false, [true, ""]];
        }
        else {
            _detectionType = "VIEW";
            _initialDetection = _detection;
            _progressDetection = false;
        };

        _detectionType = switch _detectionType do {
            case "VIEW": {
                "KH_fnc_getViewTarget";
            }; 
            
            case "WEAPON": {
                "KH_fnc_getWeaponTarget";
            }; 
            
            case "CURSOR": {
                "KH_fnc_getMouseTarget";
            };

            default {
                "KH_fnc_getViewTarget";
            };
        };

        private _argumentsReferenceId = generateUid;
        private _conditionShowReferenceId = generateUid;
        private _drawHintReferenceId = generateUid;
        private _lastProgressId = generateUid;
        private _parentId = generateUid;
        private _cancelInterruptId = generateUid;
        private _resultStartId = generateUid;
        private _resultProgressId = generateUid;
        private _resultInterruptId = generateUid;
        private _resultCancelId = generateUid;
        private _resultCompleteId = generateUid;
        missionNamespace setVariable [_argumentsReferenceId, _arguments];
        missionNamespace setVariable [_conditionShowReferenceId, _conditionShow];
        missionNamespace setVariable [_drawHintReferenceId, _drawHint];
        missionNamespace setVariable [_lastProgressId, 0];
        missionNamespace setVariable [_parentId, _parsedParent];
        missionNamespace setVariable [_cancelInterruptId, false];

        private _action = _parsedParent addAction [
            _menuName,
            {
                isNil {
                    (_this select [1]) params ["_caller", "_actionId", "_arguments"];

                    _arguments params [
                        "_menuName",
                        "_windowBackgroundName",
                        "_windowForegroundName",
                        "_progressMenuName",
                        "_progressWindowBackgroundName",
                        "_progressWindowForegroundName",
                        "_target", 
                        "_args", 
                        "_functionStart",
                        "_functionProgress",
                        "_functionInterrupt",
                        "_functionCancel",
                        "_functionComplete",
                        "_conditionStart",
                        "_conditionProgress", 
                        "_conditionComplete",
                        "_repeatable",
                        "_exclusive", 
                        "_duration",
                        "_distance",
                        "_allowIncapacitated",
                        "_progressDisplay",
                        "_resultStartId",
                        "_resultProgressId",
                        "_resultInterruptId",
                        "_resultCancelId",
                        "_resultCompleteId",
                        "_actionExistenceId",
                        "_actionSafetyId",
                        "_conditionShowId",
                        "_allowedActivationId",
                        "_completionId",
                        "_lastProgressId",
                        "_cancelInterruptId",
                        "_detectionType",
                        "_progressDetection"
                    ];
                    
                    if (!(missionNamespace getVariable _actionExistenceId) || (missionNamespace getVariable _completionId)) exitWith {
                        _caller removeAction _actionId;
                    };
                    
                    private _resultStart = nil;
                    private _resultProgress = nil;
                    private _handlerId = [missionNamespace, _actionExistenceId, true];
                    private _resultPreviousStart = missionNamespace getVariable _resultStartId;
                    private _resultPreviousProgress = missionNamespace getVariable _resultProgressId;
                    private _resultPreviousInterrupt = missionNamespace getVariable _resultInterruptId;
                    private _resultPreviousCancel = missionNamespace getVariable _resultCancelId;
                    private _resultPreviousComplete = missionNamespace getVariable _resultCompleteId;

                    if !(missionNamespace getVariable _allowedActivationId) exitWith {
                        if !(isNil {_target getVariable (["KH_var_activeAction", _actionId] joinString "_");}) then {
                            missionNamespace setVariable [_resultCancelId, _args call _functionCancel];
                            missionNamespace setVariable [_cancelInterruptId, true];
                            [_target getVariable (["KH_var_activeAction", _actionId] joinString "_")] call KH_fnc_removeHandler;
                            missionNamespace setVariable [_allowedActivationId, true];
                            missionNamespace setVariable [_conditionShowId, true, KH_var_allMachines - [clientOwner]];
                            _caller setUserActionText [_actionId, _menuName, _windowBackgroundName, _windowForegroundName];
                        };
                    };

                    if !(_args call _conditionStart) exitWith {
                        _args call _functionInterrupt;
                    };
                    
                    _resultStart = _args call _functionStart;
                    missionNamespace setVariable [_resultStartId, _resultStart];
                    missionNamespace setVariable [_resultProgressId, nil];

                    if (_exclusive || (_duration isNotEqualTo 0)) then {
                        missionNamespace setVariable [_actionSafetyId, getPlayerUID player, 2];
                        missionNamespace setVariable [_allowedActivationId, false];
                        missionNamespace setVariable [_conditionShowId, false, KH_var_allMachines - [clientOwner]];
                    };

                    if (_duration isEqualTo 0) then {
                        if (!(_args call _conditionProgress) || !(_args call _conditionComplete)) exitWith {
                            _args call _functionInterrupt;
                        };

                        _resultProgress = _args call _functionProgress;
                        _args call _functionComplete;

                        if !_repeatable then {
                            if _exclusive then {
                                [_handlerId] call KH_fnc_removeHandler;
                            }
                            else {
                                missionNamespace setVariable [_completionId, true];
                            };
                        };
                    }
                    else {
                        if !(_args call _conditionStart) exitWith {
                            _args call _functionInterrupt;
                        };

                        private _interactionHelper = createSimpleObject ["KH_HelperSquare", getPosASL _caller, true];
                        missionNamespace setVariable [_lastProgressId, 0];

                        private _durationhandler = [
                            [
                                _target,
                                _menuName,
                                _windowBackgroundName,
                                _windowForegroundName,
                                _progressMenuName,
                                _progressWindowBackgroundName,
                                _progressWindowForegroundName,
                                _caller,
                                _args, 
                                _functionProgress, 
                                _functionInterrupt, 
                                _functionComplete, 
                                _conditionProgress, 
                                _conditionComplete,
                                _repeatable,
                                _exclusive,
                                _duration,
                                _distance,
                                _allowIncapacitated,
                                _progressDisplay,
                                _resultProgressId,
                                _resultInterruptId,
                                _resultCompleteId,
                                _resultPreviousStart,
                                _resultPreviousProgress,
                                _resultPreviousInterrupt,
                                _resultPreviousCancel,
                                _resultPreviousComplete,
                                _actionExistenceId,
                                _actionSafetyId,
                                _conditionShowId,
                                _allowedActivationId,
                                _completionId,
                                _lastProgressId,
                                _cancelInterruptId,
                                _detectionType,
                                _progressDetection,
                                _actionId,
                                _handlerId,
                                _interactionHelper,
                                time,
                                time + _duration,
                                _resultStart
                            ],
                            {
                                params [
                                    "_target",
                                    "_menuName",
                                    "_windowBackgroundName",
                                    "_windowForegroundName",
                                    "_progressMenuName",
                                    "_progressWindowBackgroundName",
                                    "_progressWindowForegroundName",
                                    "_caller",
                                    "_arguments", 
                                    "_functionProgress",
                                    "_functionInterrupt",
                                    "_functionComplete",
                                    "_conditionProgress", 
                                    "_conditionComplete",
                                    "_repeatable",
                                    "_exclusive", 
                                    "_duration",
                                    "_distance",
                                    "_allowIncapacitated",
                                    "_progressDisplay",
                                    "_resultProgressId",
                                    "_resultInterruptId",
                                    "_resultCompleteId",
                                    "_resultPreviousStart",
                                    "_resultPreviousProgress",
                                    "_resultPreviousInterrupt",
                                    "_resultPreviousCancel",
                                    "_resultPreviousComplete",
                                    "_actionExistenceId",
                                    "_actionSafetyId",
                                    "_conditionShowId",
                                    "_allowedActivationId",
                                    "_completionId",
                                    "_lastProgressId",
                                    "_cancelInterruptId",
                                    "_detectionType",
                                    "_progressDetection",
                                    "_actionId",
                                    "_handlerId",
                                    "_interactionHelper",
                                    "_startTime",
                                    "_endTime",
                                    "_resultStart"
                                ];

                                private _resultProgress = missionNamespace getVariable _resultProgressId;

                                if (_progressDisplay isNotEqualTo "NONE") then {
                                    private _progress = if (_duration isEqualTo true) then {
                                        "ACTIVE";
                                    }
                                    else {
                                        private _progressColor = [
                                            profileNamespace getVariable ["GUI_BCG_RGB_R", 0.13], 
                                            profileNamespace getVariable ["GUI_BCG_RGB_G", 0.54], 
                                            profileNamespace getVariable ["GUI_BCG_RGB_B", 0.21], 
                                            1
                                        ] call BIS_fnc_colorRGBAtoHTML;

                                        switch (_progressDisplay) do {
                                            case "PERCENTAGE": {
                                                private _currentProgress = ((parseNumber ((((time - _startTime) / (_endTime - _startTime)) * 100) toFixed 1)) min 100) toFixed 1;
                                                missionNamespace setVariable [_lastProgressId, _currentProgress];
                                                ["<t size='1.25' color='", _progressColor, "' font='EtelkaMonospaceProBold'>", _currentProgress, "%</t>"] joinString "";
                                            };

                                            case "TIME": {
                                                private _currentProgress = ((parseNumber ((_endTime - time) toFixed 1)) max 0) toFixed 1;
                                                missionNamespace setVariable [_lastProgressId, _currentProgress];
                                                ["<t size='1.25' color='", _progressColor, "' font='EtelkaMonospaceProBold'>", _currentProgress, "s</t>"] joinString "";
                                            };

                                            case "BAR": {
                                                private _currentProgress = floor (((time - _startTime) / (_endTime - _startTime)) * 100);
                                                private _bars = [];

                                                for "_i" from 1 to 100 do {
                                                    if (_i <= _currentProgress) then {
                                                        _bars pushBack "|";
                                                    }
                                                    else {
                                                        _bars pushBack " ";
                                                    };
                                                };

                                                missionNamespace setVariable [_lastProgressId, _currentProgress];
                                                ["<t size='0.75' color='", _progressColor, "' font='EtelkaMonospaceProBold'>", (str _bars) regexReplace ["['"",]", ""], "</t>"] joinString "";
                                            };

                                            default {
                                                ["<t size='1.25' color='", _progressColor, "' font='EtelkaMonospaceProBold'>PROGRESS UNKNOWN</t>"] joinString "";
                                            };
                                        };
                                    };

                                    _caller setUserActionText [_actionId, [_progressMenuName, _progress] joinString " ", [_progressWindowBackgroundName, "<br/>", _progress] joinString "", _progressWindowForegroundName];
                                };

                                missionNamespace setVariable [_resultProgressId, _arguments call _functionProgress];
                            },
                            true,
                            [
                                {
                                    params [
                                        "_target",
                                        "_menuName",
                                        "_windowBackgroundName",
                                        "_windowForegroundName",
                                        "_progressMenuName",
                                        "_progressWindowBackgroundName",
                                        "_progressWindowForegroundName",
                                        "_caller", 
                                        "_arguments", 
                                        "_functionProgress",
                                        "_functionInterrupt",
                                        "_functionComplete",
                                        "_conditionProgress", 
                                        "_conditionComplete",
                                        "_repeatable",
                                        "_exclusive", 
                                        "_duration",
                                        "_distance",
                                        "_allowIncapacitated",
                                        "_progressDisplay",
                                        "_resultProgressId",
                                        "_resultInterruptId",
                                        "_resultCompleteId",
                                        "_resultPreviousStart",
                                        "_resultPreviousProgress",
                                        "_resultPreviousInterrupt",
                                        "_resultPreviousCancel",
                                        "_resultPreviousComplete",
                                        "_actionExistenceId",
                                        "_actionSafetyId",
                                        "_conditionShowId",
                                        "_allowedActivationId",
                                        "_completionId",
                                        "_lastProgressId",
                                        "_cancelInterruptId",
                                        "_detectionType",
                                        "_progressDetection",
                                        "_actionId",
                                        "_handlerId",
                                        "_interactionHelper",
                                        "_startTime",
                                        "_endTime",
                                        "_resultStart"
                                    ];

                                    if (
                                        !(alive _caller) || 
                                        (_caller isNotEqualTo KH_var_playerUnit) || 
                                        !(missionNamespace getVariable _actionExistenceId) ||
                                        !(missionNamespace getVariable _conditionShowId) ||
                                        (missionNamespace getVariable _completionId) ||
                                        ((((lifeState _caller) isEqualTo "INCAPACITATED") || ((lifeState _caller) isEqualTo "UNCONSCIOUS")) && !_allowIncapacitated)
                                       ) exitWith {
                                        deleteVehicle _interactionHelper;
                                        false;
                                    };

                                    private _resultProgress = missionNamespace getVariable _resultProgressId;

                                    if (_target isEqualTo _caller) exitWith {
                                        deleteVehicle _interactionHelper;
                                        _arguments call _conditionProgress;
                                    };

                                    switch _progressDetection do {
                                        case true: {
                                            deleteVehicle _interactionHelper;
                                            private _viewTarget = call (missionNamespace getVariable _detectionType);
                                            
                                            if (((_viewTarget select 1) > _distance) || ((_viewTarget select 4) isNotEqualTo _target)) then {
                                                false;
                                            }
                                            else {
                                                _arguments call _conditionProgress;
                                            };
                                        };

                                        case false: {
                                            if ((_interactionHelper distance _caller) > _distance) then {
                                                deleteVehicle _interactionHelper;
                                                false;
                                            }
                                            else {
                                                _arguments call _conditionProgress;
                                            };
                                        };

                                        default {
                                            deleteVehicle _interactionHelper;
                                            private _viewTarget = call (missionNamespace getVariable _detectionType);
                                            
                                            if (((_viewTarget select 1) > _distance) || ((_viewTarget select 5) isNotEqualTo _progressDetection)) then {
                                                false;
                                            }
                                            else {
                                                _arguments call _conditionProgress;
                                            };
                                        };
                                    };
                                },
                                true,
                                0,
                                [_duration, false, true, false],
                                {
                                    params [
                                        "_target",
                                        "_menuName",
                                        "_windowBackgroundName",
                                        "_windowForegroundName",
                                        "_progressMenuName",
                                        "_progressWindowBackgroundName",
                                        "_progressWindowForegroundName",
                                        "_caller", 
                                        "_arguments", 
                                        "_functionProgress",
                                        "_functionInterrupt",
                                        "_functionComplete",
                                        "_conditionProgress", 
                                        "_conditionComplete",
                                        "_repeatable",
                                        "_exclusive", 
                                        "_duration",
                                        "_distance",
                                        "_allowIncapacitated",
                                        "_progressDisplay",
                                        "_resultProgressId",
                                        "_resultInterruptId",
                                        "_resultCompleteId",
                                        "_resultPreviousStart",
                                        "_resultPreviousProgress",
                                        "_resultPreviousInterrupt",
                                        "_resultPreviousCancel",
                                        "_resultPreviousComplete",
                                        "_actionExistenceId",
                                        "_actionSafetyId",
                                        "_conditionShowId",
                                        "_allowedActivationId",
                                        "_completionId",
                                        "_lastProgressId",
                                        "_cancelInterruptId",
                                        "_detectionType",
                                        "_progressDetection",
                                        "_actionId",
                                        "_handlerId",
                                        "_interactionHelper",
                                        "_startTime",
                                        "_endTime",
                                        "_resultStart"
                                    ];
                                    
                                    deleteVehicle _interactionHelper;
                                    missionNamespace setVariable [_allowedActivationId, true];

                                    if (_exclusive || (_duration isNotEqualTo 0)) then {
                                        missionNamespace setVariable [_conditionShowId, true, true];
                                    };

                                    private _resultProgress = missionNamespace getVariable _resultProgressId;

                                    if (_conditionFailure || !(_arguments call _conditionComplete)) then {
                                        if (missionNamespace getVariable _cancelInterruptId) then {
                                            missionNamespace setVariable [_cancelInterruptId, false];
                                            missionNamespace setVariable [_resultInterruptId, _arguments call _functionInterrupt];
                                        };
                                    }
                                    else {
                                        if !_repeatable then {
                                            if _exclusive then {
                                                [_handlerId] call KH_fnc_removeHandler;
                                            }
                                            else {
                                                missionNamespace setVariable [_completionId, true];
                                            };
                                        };

                                        missionNamespace setVariable [_resultCompleteId, _arguments call _functionComplete];
                                    };

                                    _caller setUserActionText [_actionId, _menuName, _windowBackgroundName, _windowForegroundName];
                                },
                                false,
                                true
                            ],
                            false
                        ] call KH_fnc_execute;

                        _target setVariable [["KH_var_activeAction", _actionId] joinString "_", _durationHandler];
                    };
                };

                nil;
            },
            [
                _menuName,
                _windowBackgroundName,
                _windowForegroundName,
                _progressMenuName,
                _progressWindowBackgroundName,
                _progressWindowForegroundName,
                _parsedObject,
                _arguments,
                _functionStart, 
                _functionProgress, 
                _functionInterrupt,
                _functionCancel,
                _functionComplete,
                _conditionStart,
                _conditionProgress, 
                _conditionComplete, 
                _repeatable,
                _exclusive, 
                _duration, 
                _distance,
                _allowIncapacitated,
                _progressDisplay,
                _resultStartId,
                _resultProgressId,
                _resultInterruptId,
                _resultCancelId,
                _resultCompleteId,
                _actionExistenceId,
                _actionSafetyId,
                _conditionShowId,
                _allowedActivationId,
                _completionId,
                _lastProgressId,
                _cancelInterruptId,
                _detectionType,
                _progressDetection
            ],
            1.5,
            _showImmediately,
            _hideOnUse,
            _userInput,
            if _visiblePerPlayer then {
                [
                    "if (!(missionNamespace getVariable '", _actionExistenceId, "') || (missionNamespace getVariable '", _completionId, "')) exitWith {
                        (missionNamespace getVariable '", _parentId, "') removeAction _actionId;
                        false;
                    };

                    if ((missionNamespace getVariable '", _parentId, "') isNotEqualTo KH_var_playerUnit) exitWith {
                        false;
                    };

                    (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'));"
                ] joinString "";
            }
            else {
                switch _initialDetection do {
                    case true: {
                        [
                            "if (!(missionNamespace getVariable '", _actionExistenceId, "') || (isNull (missionNamespace getVariable '", _objectId, "')) || (missionNamespace getVariable '", _completionId, "')) exitWith {
                                (missionNamespace getVariable '", _parentId, "') removeAction _actionId;
                                false;
                            };

                            if ((missionNamespace getVariable '", _parentId, "') isNotEqualTo KH_var_playerUnit) exitWith {
                                false;
                            };

                            if ((missionNamespace getVariable '", _objectId, "') isEqualTo KH_var_playerUnit) exitWith {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };
                                
                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            };

                            private _viewTarget = call ", _detectionType, ";

                            if (((_viewTarget select 1) <= ", _distance, ") && ((_viewTarget select 4) isEqualTo (missionNamespace getVariable '", _objectId, "'))) then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };
                                
                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'));
                            } 
                            else {
                                false;
                            }"
                        ] joinString "";
                    };

                    case false: {
                        [
                            "if (!(missionNamespace getVariable '", _actionExistenceId, "') || (isNull (missionNamespace getVariable '", _objectId, "')) || (missionNamespace getVariable '", _completionId, "')) exitWith {
                                (missionNamespace getVariable '", _parentId, "') removeAction _actionId;
                                false;
                            };

                            if ((missionNamespace getVariable '", _parentId, "') isNotEqualTo KH_var_playerUnit) exitWith {
                                false;
                            };

                            if ((missionNamespace getVariable '", _objectId, "') isEqualTo KH_var_playerUnit) exitWith {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };
                                
                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            };

                            if (((missionNamespace getVariable '", _objectId, "') distance (missionNamespace getVariable '", _parentId, "')) <= ", _distance, ") then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];
                            
                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };

                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'));
                            }
                            else {
                                false;
                            }"
                        ] joinString "";
                    };

                    default {
                        [
                            "if (!(missionNamespace getVariable '", _actionExistenceId, "') || (isNull (missionNamespace getVariable '", _objectId, "')) || (missionNamespace getVariable '", _completionId, "')) exitWith {
                                (missionNamespace getVariable '", _parentId, "') removeAction _actionId;
                                false;
                            };

                            if ((missionNamespace getVariable '", _parentId, "') isNotEqualTo KH_var_playerUnit) exitWith {
                                false;
                            };

                            if ((missionNamespace getVariable '", _objectId, "') isEqualTo KH_var_playerUnit) exitWith {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };
                                
                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            };

                            private _viewTarget = call ", _detectionType, ";

                            if (
                                ((_viewTarget select 1) <= ", _distance, ") && 
                                (((_viewTarget select 5) param [0, '']) isEqualTo '", _initialDetection, "') && 
                                ((_viewTarget select 4) isEqualTo (missionNamespace getVariable '", _objectId, "'))
                            ) then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };

                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'));
                            }
                            else {
                                false;
                            }"
                        ] joinString "";
                    };
                };
            },
            0,
            _allowIncapacitated,
            "",
            ""
        ];

        _parsedParent setUserActionText [_action, _menuName, _windowBackgroundName, _windowForegroundName];
        private _currentTarget = missionNamespace getVariable [_targetId, []]; 
        _currentTarget insert [-1, [[_target], [clientOwner]] select ((_target isEqualTo true) || (_target isEqualTo "LOCAL")), true];
        missionNamespace setVariable [_targetId, _currentTarget];

        if _handleParentActionRecovery then {
            if !_playerOnly then {
                [
                    "PLAYER",
                    "unit",
                    [
                        _object,
                        _name, 
                        _arguments, 
                        _function, 
                        _condition, 
                        _repeatable, 
                        _exclusive,
                        _duration, 
                        _distance,
                        _showImmediately,
                        _allowIncapacitated,
                        _hideOnUse,
                        _drawHint, 
                        _detection, 
                        _userInput,
                        _progressDisplay,
                        _parsedIdentifier,
                        _actionExistenceId,
                        []
                    ],
                    {
                        params ["_unit"];

                        _args params [
                            "_object",
                            "_name", 
                            "_arguments", 
                            "_function", 
                            "_condition", 
                            "_repeatable", 
                            "_exclusive",
                            "_duration", 
                            "_distance",
                            "_showImmediately",
                            "_allowIncapacitated",
                            "_hideOnUse",
                            "_drawHint", 
                            "_detection", 
                            "_userInput", 
                            "_progressDisplay",
                            "_parsedIdentifier",
                            "_actionExistenceId",
                            "_controlledUnitActions"
                        ];

                        if !(missionNamespace getVariable _actionExistenceId) exitWith {
                            [_handlerId] call KH_fnc_removeHandler;
                        };
                        
                        if !(_unit in _controlledUnitActions) then {
                            _controlledUnitActions pushBack _unit;

                            [
                                [_object, false],
                                _name, 
                                _arguments, 
                                _function, 
                                _condition, 
                                _repeatable, 
                                _exclusive,
                                [true, false],
                                [_duration, _progressDisplay],
                                _distance,
                                _showImmediately,
                                _allowIncapacitated,
                                _hideOnUse,
                                _drawHint, 
                                _detection, 
                                _userInput, 
                                _parsedIdentifier,
                                [_unit, false]
                            ] call KH_fnc_addAction;
                        };
                    }
                ] call KH_fnc_addEventHandler;
            };
        };

        if (_handleObjectActionRecovery && _visiblePerPlayer) then {
            [
                ["ENTITY", player, "LOCAL"],
                "Respawn",
                [
                    _name, 
                    _arguments, 
                    _function, 
                    _condition, 
                    _repeatable, 
                    _exclusive,
                    _duration, 
                    _distance,
                    _showImmediately,
                    _allowIncapacitated,
                    _hideOnUse,
                    _drawHint, 
                    _detection, 
                    _userInput, 
                    _progressDisplay,
                    _parsedIdentifier,
                    _actionExistenceId,
                    _parent,
                    _handleParentActionRecovery,
                    _targetId
                ],
                {
                    params ["_unit"];

                    _args params [
                        "_name", 
                        "_arguments", 
                        "_function", 
                        "_condition", 
                        "_repeatable", 
                        "_exclusive",
                        "_duration", 
                        "_distance",
                        "_showImmediately",
                        "_allowIncapacitated",
                        "_hideOnUse",
                        "_drawHint", 
                        "_detection", 
                        "_userInput", 
                        "_progressDisplay",
                        "_parsedIdentifier",
                        "_actionExistenceId",
                        "_parent",
                        "_handleParentActionRecovery",
                        "_targetId"
                    ];

                    if !(missionNamespace getVariable _actionExistenceId) exitWith {
                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    [
                        [_unit, false],
                        _name, 
                        _arguments, 
                        _function, 
                        _condition, 
                        _repeatable, 
                        _exclusive,
                        [missionNamespace getVariable _targetId, _jip],
                        [_duration, _progressDisplay],
                        _distance,
                        _showImmediately,
                        _allowIncapacitated,
                        _hideOnUse,
                        _drawHint, 
                        _detection, 
                        _userInput, 
                        _parsedIdentifier,
                        [_parent, _handleParentActionRecovery]
                    ] call KH_fnc_addAction;

                    nil;
                }
            ] call KH_fnc_addEventHandler;
        };

        (missionNamespace getVariable _actionStorageId) pushBack [_parent, _action];
        nil;
    },
    [_target, false] select ((_target isEqualTo "PLAYERS") && !(_object isEqualType true)),
    true,
    [[false, ["JIP", _object, true, ""]] select _jip, ["NEAR_PLAYERS", _object, _distance + 100, _jip, ""]] select ((_target isEqualTo "PLAYERS") && !(_object isEqualType true))
] call KH_fnc_execute;

[
    [
        _object,
        _name, 
        _arguments, 
        _function, 
        _condition, 
        _repeatable, 
        _exclusive,
        _target,
        _targetId,
        _jip,
        _duration, 
        _distance,
        _showImmediately,
        _allowIncapacitated,
        _hideOnUse,
        _drawHint, 
        _detection, 
        _userInput, 
        _progressDisplay,
        _parsedIdentifier,
        _parent,
        _handleObjectActionRecovery,
        _handleParentActionRecovery,
        _actionExistenceId,
        _actionSafetyId,
        _actionStorageId,
        _conditionShowId,
        _actionHandler,
        _firstCall
    ],
    {
        params [
            "_object",
            "_name", 
            "_arguments", 
            "_function", 
            "_condition", 
            "_repeatable", 
            "_exclusive",
            "_target",
            "_targetId",
            "_jip",
            "_duration", 
            "_distance",
            "_showImmediately",
            "_allowIncapacitated",
            "_hideOnUse",
            "_drawHint", 
            "_detection", 
            "_userInput", 
            "_progressDisplay",
            "_parsedIdentifier",
            "_parent",
            "_handleObjectActionRecovery",
            "_handleParentActionRecovery",
            "_actionExistenceId",
            "_actionSafetyId",
            "_actionStorageId",
            "_conditionShowId",
            "_actionHandler",
            "_firstCall"
        ];

        private _currentTarget = missionNamespace getVariable [_targetId, []]; 
        _currentTarget insert [-1, [[_target], [clientOwner]] select ((_target isEqualTo true) || (_target isEqualTo "LOCAL")), true];
        missionNamespace setVariable [_targetId, _currentTarget];
        private _actionCleanupHandlerId = ["KH_var_actionCleanupHandlerId", _parsedIdentifier] joinString "_";
        private _actionCleanupHandler = missionNamespace getVariable _actionCleanupHandlerId;

        if _firstCall then {
            if !(_object isEqualType true) then {
                if _handleObjectActionRecovery then {
                    [
                        ["ENTITY", _object, "REMOTE"],
                        "Respawn",
                        [
                            _name, 
                            _arguments, 
                            _function, 
                            _condition, 
                            _repeatable, 
                            _exclusive,
                            _targetId,
                            _jip,
                            _duration, 
                            _distance,
                            _showImmediately,
                            _allowIncapacitated,
                            _hideOnUse,
                            _drawHint, 
                            _detection, 
                            _userInput, 
                            _progressDisplay,
                            _parsedIdentifier,
                            _parent,
                            _handleParentActionRecovery,
                            _actionExistenceId
                        ],
                        {
                            params ["_unit"];

                            _args params [
                                "_name", 
                                "_arguments", 
                                "_function", 
                                "_condition", 
                                "_repeatable", 
                                "_exclusive",
                                "_targetId",
                                "_jip",
                                "_duration", 
                                "_distance",
                                "_showImmediately",
                                "_allowIncapacitated",
                                "_hideOnUse",
                                "_drawHint", 
                                "_detection", 
                                "_userInput", 
                                "_progressDisplay",
                                "_parsedIdentifier",
                                "_parent",
                                "_handleParentActionRecovery",
                                "_actionExistenceId"
                            ];

                            if !(missionNamespace getVariable _actionExistenceId) exitWith {
                                [_handlerId] call KH_fnc_removeHandler;
                            };

                            [
                                [_unit, false],
                                _name, 
                                _arguments, 
                                _function, 
                                _condition, 
                                _repeatable, 
                                _exclusive,
                                [missionNamespace getVariable _targetId, _jip],
                                [_duration, _progressDisplay], 
                                _distance,
                                _showImmediately,
                                _allowIncapacitated,
                                _hideOnUse,
                                _drawHint, 
                                _detection, 
                                _userInput, 
                                _parsedIdentifier,
                                [_parent, _handleParentActionRecovery]
                            ] call KH_fnc_addAction;
                        }
                    ] call KH_fnc_addEventHandler;
                };
            };

            [
                "CBA",
                "KH_eve_playerDisconnected",
                [_actionSafetyId, _conditionShowId, _actionExistenceId],
                {
                    private _uid = param [1];
                    _args params ["_actionSafetyId", "_conditionShowId", "_actionExistenceId"];

                    if !(missionNamespace getVariable _actionExistenceId) exitWith {
                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    private _currentInteractingPlayer = missionNamespace getVariable _actionSafetyId;

                    if !(isNil "_currentInteractingPlayer") then {
                        if (_currentInteractingPlayer isEqualTo _uid) then {
                            missionNamespace setVariable [_conditionShowId, true, true];
                        };
                    };
                }
            ] call KH_fnc_addEventHandler;

            _actionCleanupHandler = [_actionHandler];
            missionNamespace setVariable [_actionCleanupHandlerId, _actionCleanupHandler];

            [
                "CBA",
                "KH_eve_handlerRemoved",
                [_actionCleanupHandlerId, _actionExistenceId, _currentTarget, _actionStorageId],
                {
                    params ["_handler"];
                    _args params ["_actionCleanupHandlerId", "_actionExistenceId", "_currentTarget", "_actionStorageId"];

                    if ((_handler select 1) isEqualTo _actionExistenceId) then {
                        {
                            [_x] call KH_fnc_removeHandler;
                        } forEach _actionCleanupHandlerId;

                        [
                            [_actionStorageId],
                            {
                                params ["_actionStorageId"];
                                {
                                    (_x select 0) removeAction (_x select 1);
                                } forEach (missionNamespace getVariable _actionStorageId);
                            },
                            _currentTarget,
                            true,
                            false
                        ] call KH_fnc_execute;

                        ["KH_eve_handlerRemoved", _handlerId select 2] call CBA_fnc_removeEventHandler;
                    };
                }
            ] call KH_fnc_addEventHandler;
        }
        else {
            _actionCleanupHandler pushBack _actionHandler;
        };
    },
    "SERVER",
    true,
    false
] call KH_fnc_execute;

if (_firstCall && !(isNil "_parsedConditionExist")) then {
    private "_objectId";

    if !(_object isEqualType true) then {
        _objectId = generateUid;
        missionNamespace setVariable [_objectId, _object];
    };

    [
        [
            _arguments, 
            _parsedConditionExist, 
            _actionExistenceId
        ],
        {
            private _actionExistenceId = param [2];

            if (missionNamespace getVariable _actionExistenceId) then {
                [[missionNamespace, _actionExistenceId, true]] call KH_fnc_removeHandler;
            };
        },
        true,
        {
            params ["_arguments", "_condition", "_actionExistenceId"];
            (!(_arguments call _condition) || !(missionNamespace getVariable _actionExistenceId));
        },
        false
    ] call KH_fnc_execute;
};

[missionNamespace, _actionExistenceId, true];