params [
    ["_object", objNull, [objNull]],
    ["_name", "", ["", []]],
    ["_arguments", []],
    ["_function", {}, [[], {}]],
    ["_condition", {true;}, [[], {}]],
    ["_repeatable", false, [true]],
    ["_exclusive", true, [true]],
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
    ["_jip", true, [true]],
    ["_duration", 0, [0]],
    ["_distance", 3, [0]],
    ["_showImmediately", true, [true]],
    ["_showUnconscious", false, [true]],
    ["_hideOnUse", false, [true]],
    ["_drawHint", false, [true, []]],
    ["_detection", true, [true, ""]],
    ["_userInput", "", [""]],
    ["_progressDisplay", "BAR", [""]],
    ["_identifier", "", [""]]
];

if _jip then {
    _jip = ["JIP", _object, true, ""];
};

if (_drawHint isEqualTo false) then {
    _drawHint = [];
}
else {
    if (_drawHint isEqualTo true) then {
        _drawHint = [
            "ICON",
            [
                "\x\kh\addons\main\ui\helper_arrow_up.paa",
                [profileNamespace getVariable ['GUI_BCG_RGB_R', 0.13], profileNamespace getVariable ['GUI_BCG_RGB_G', 0.54], profileNamespace getVariable ['GUI_BCG_RGB_B', 0.21], 0.5],
                if (_detection isEqualType "") then {
                    _object modelToWorldVisual (_object selectionPosition _detection);
                }
                else {
                    unitAimPositionVisual _object;
                },
                0.5,
                0.5,
                1,
                if (_name isEqualType "") then {
                    _name;
                }
                else {
                    _name select 0;
                },
                true,
                0.04,
                "RobotoCondensedBold",
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

_duration = _duration max 0;

private _parsedIdentifier = if (_identifier isEqualTo "") then {
    generateUid;
}
else {
    _identifier;
};

private _parsedActionId = ["KH_var_actionId", _parsedIdentifier] joinString "_";
private _actionExistenceId = ["KH_var_actionExistenceId", _parsedIdentifier] joinString "_";
private _conditionShowId = ["KH_var_actionShowId", _parsedIdentifier] joinString "_";

if (isNil {missionNamespace getVariable _parsedActionId;}) then {
    missionNamespace setVariable [_parsedActionId, true, true];
    missionNamespace setVariable [_actionExistenceId, true, true];
    missionNamespace setVariable [_conditionShowId, true, true];
};

[
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
        _showUnconscious,
        _hideOnUse,
        _drawHint, 
        _detection, 
        _userInput, 
        _progressDisplay, 
        _actionExistenceId, 
        _conditionShowId,
        _parsedActionId
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
            "_duration", 
            "_distance",
            "_showImmediately",
            "_showUnconscious",
            "_hideOnUse",
            "_drawHint",
            "_detection",
            "_userInput",
            "_progressDisplay",
            "_actionExistenceId",
            "_conditionShowId",
            "_parsedActionId"
        ];

        private _allowedActivationId = generateUid;
        missionNamespace setVariable [_allowedActivationId, true];
        private "_menuName";
        private "_windowBackgroundName";
        private "_windowForegroundName";
        private "_progressMenuName";
        private "_progressWindowBackgroundName";
        private "_progressWindowForegroundName";
        private "_conditionShow";
        private "_conditionProgress";
        private "_conditionComplete";
        private _functionStart = {};
        private _functionProgress = {};
        private _functionInterrupt = {};
        private "_functionComplete";

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

        if (_condition isEqualType {}) then {
            _conditionShow = _condition;
            _conditionProgress = _condition;
            _conditionComplete = _condition;
        }
        else {
            _conditionShow = _condition param [0, {true;}, [{}]];
            _conditionProgress = _condition param [1, {true;}, [{}]];
            _conditionComplete = _condition param [2, {true;}, [{}]];
        };

        if (_function isEqualType {}) then {
            _functionComplete = _function;
        }
        else {
            _functionStart = _function param [0, _functionStart, [{}]];
            _functionProgress = _function param [1, _functionProgress, [{}]];
            _functionInterrupt = _function param [2, _functionInterrupt, [{}]];
            _functionComplete = _function param [3, {}];
        };

        private _argumentsReferenceId = generateUid;
        private _conditionShowReferenceId = generateUid;
        private _drawHintReferenceId = generateUid;
        private _lastProgressId = generateUid;
        missionNamespace setVariable [_argumentsReferenceId, _arguments];
        missionNamespace setVariable [_conditionShowReferenceId, _conditionShow];
        missionNamespace setVariable [_drawHintReferenceId, _drawHint];
        missionNamespace setVariable [_lastProgressId, _lastProgressId];

        private _action = player addAction [
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
                        "_functionComplete",
                        "_conditionProgress", 
                        "_conditionComplete",
                        "_repeatable",
                        "_exclusive", 
                        "_duration",
                        "_distance",
                        "_progressDisplay",
                        "_actionExistenceId",
                        "_conditionShowId",
                        "_allowedActivationId",
                        "_lastProgressId"
                    ];
                    
                    if !(missionNamespace getVariable _actionExistenceId) exitWith {
                        player removeAction _actionId;
                    };

                    if !(missionNamespace getVariable _allowedActivationId) exitWith {
                        if !(isNil {missionNamespace getVariable (["KH_var_activeAction", _target, _actionId] joinString "_");}) then {
                            [missionNamespace getVariable (["KH_var_activeAction", _target, _actionId] joinString "_")] call KH_fnc_removeHandler;
                            missionNamespace setVariable [_allowedActivationId, true];
                            missionNamespace setVariable [_conditionShowId, true, KH_var_allMachines - [clientOwner]];
                            player setUserActionText [_actionId, _menuName, _windowBackgroundName, _windowForegroundName];
                        };
                    };

                    private _handlerId = [missionNamespace, _actionExistenceId, true];

                    if (_exclusive || (_duration isNotEqualTo 0)) then {
                        missionNamespace setVariable [_allowedActivationId, false];
                        missionNamespace setVariable [_conditionShowId, false, KH_var_allMachines - [clientOwner]];
                    };

                    if (_duration isEqualTo 0) then {
                        if (!(_args call _conditionProgress) || !(_args call _conditionComplete)) exitWith {};
                        _args call _functionStart;
                        _args call _functionProgress;
                        _args call _functionComplete;

                        if !_repeatable then {
                            missionNamespace setVariable [_actionExistenceId, false, _exclusive];
                        };
                    }
                    else {
                        if !(_args call _conditionProgress) exitWith {
                            _args call _functionInterrupt;
                        };

                        private _interactionHelper = createSimpleObject ["KH_HelperSquare", getPosASL _caller, true];
                        missionNamespace setVariable [_lastProgressId, 0];
                        _args call _functionStart;

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
                                _progressDisplay,
                                _actionExistenceId, 
                                _conditionShowId,
                                _allowedActivationId,
                                _lastProgressId,
                                _actionId, 
                                _interactionHelper,
                                time,
                                time + _duration
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
                                    "_progressDisplay",
                                    "_actionExistenceId",
                                    "_conditionShowId",
                                    "_allowedActivationId",
                                    "_lastProgressId",
                                    "_actionId",
                                    "_interactionHelper",
                                    "_startTime",
                                    "_endTime"
                                ];

                                private _handlerId = [[missionNamespace, _actionExistenceId, true], _handlerId];

                                if (_progressDisplay isNotEqualTo "NONE") then {
                                    private _progress = switch (_progressDisplay) do {
                                        case "PERCENTAGE": {
                                            private _currentProgress = ((parseNumber ((((time - _startTime) / (_endTime - _startTime)) * 100) toFixed 1)) min 100) toFixed 1;
                                            missionNamespace setVariable [_lastProgressId, _currentProgress];
                                            ["<t size='1.25' font='EtelkaMonospaceProBold'>", _currentProgress, "%</t>"] joinString "";
                                        };

                                        case "TIME": {
                                            private _currentProgress = ((parseNumber ((_endTime - time) toFixed 1)) max 0) toFixed 1;
                                            missionNamespace setVariable [_lastProgressId, _currentProgress];
                                            ["<t size='1.25' font='EtelkaMonospaceProBold'>", _currentProgress, "s</t>"] joinString "";
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
                                            ["<t size='0.75' font='EtelkaMonospaceProBold'>", (str _bars) regexReplace ["['"",]", ""], "</t>"] joinString "";
                                        };

                                        default {
                                            "<t size='1.25' font='EtelkaMonospaceProBold'>PROGRESS UNKNOWN</t>";
                                        };
                                    };

                                    player setUserActionText [_actionId, [_progressMenuName, _progress] joinString " ", [_progressWindowBackgroundName, "<br/>", _progress] joinString "", _progressWindowForegroundName];
                                };

                                _arguments call _functionProgress;
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
                                        "_progressDisplay",
                                        "_actionExistenceId",
                                        "_conditionShowId",
                                        "_allowedActivationId",
                                        "_lastProgressId",
                                        "_actionId",
                                        "_interactionHelper",
                                        "_startTime",
                                        "_endTime"
                                    ];
                                    
                                    if ((_interactionHelper distance _caller) > _distance) then {
                                        deleteVehicle _interactionHelper;
                                        false;
                                    }
                                    else {
                                        _arguments call _conditionProgress;
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
                                        "_progressDisplay",
                                        "_actionExistenceId",
                                        "_conditionShowId",
                                        "_allowedActivationId",
                                        "_lastProgressId",
                                        "_actionId",
                                        "_interactionHelper",
                                        "_startTime",
                                        "_endTime"
                                    ];
                                    
                                    deleteVehicle _interactionHelper;
                                    private _handlerId = [[missionNamespace, _actionExistenceId, true], _handlerId];
                                    missionNamespace setVariable [_allowedActivationId, true];

                                    if (_exclusive || (_duration isNotEqualTo 0)) then {
                                        missionNamespace setVariable [_conditionShowId, true, true];
                                    };

                                    if (_conditionFailure || !(_arguments call _conditionComplete)) then {
                                        _arguments call _functionInterrupt;
                                    }
                                    else {
                                        if !_repeatable then {
                                            missionNamespace setVariable [_actionExistenceId, false, _exclusive];
                                        };

                                        _arguments call _functionComplete;
                                    };

                                    player setUserActionText [_actionId, _menuName, _windowBackgroundName, _windowForegroundName];
                                },
                                false,
                                true
                            ],
                            false
                        ] call KH_fnc_execute;

                        missionNamespace setVariable [["KH_var_activeAction", _target, _actionId] joinString "_", _durationHandler];
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
                _object,
                _arguments,
                _functionStart, 
                _functionProgress, 
                _functionInterrupt, 
                _functionComplete,
                _conditionProgress, 
                _conditionComplete, 
                _repeatable,
                _exclusive, 
                _duration, 
                _distance,
                _progressDisplay,
                _actionExistenceId,
                _conditionShowId,
                _allowedActivationId,
                _lastProgressId
            ],
            1.5,
            _showImmediately,
            _hideOnUse,
            _userInput,
            if (_object isEqualTo player) then {
                private _objectId = generateUid;
                missionNamespace setVariable [_objectId, _object, true];

                [
                    "if !(missionNamespace getVariable '", _actionExistenceId, "') exitWith {
                        player removeAction _actionId;
                        false;
                    };

                    if ((missionNamespace getVariable '", _objectId, "') isNotEqualTo player) exitWith {
                        private _viewTarget = call KH_fnc_getViewTarget;

                        if (((_viewTarget select 1) <= ", _distance, ") && ((_viewTarget select 4) isEqualTo (missionNamespace getVariable '", _objectId, "'))) then {
                            private _caller = _this;
                            private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                            if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                            };
                            
                            (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                        } 
                        else {
                            false;
                        };
                    };

                    private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];
                    private _caller = _this;
                    (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))"
                ] joinString "";
            }
            else {
                switch _detection do {
                    case true: { 
                        private _objectId = generateUid;
                        missionNamespace setVariable [_objectId, _object, true];

                        [
                            "if !(missionNamespace getVariable '", _actionExistenceId, "') exitWith {
                                player removeAction _actionId;
                                false;
                            };

                            private _viewTarget = call KH_fnc_getViewTarget;

                            if (((_viewTarget select 1) <= ", _distance, ") && ((_viewTarget select 4) isEqualTo (missionNamespace getVariable '", _objectId, "'))) then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };
                                
                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            } 
                            else {
                                false;
                            }"
                        ] joinString "";
                    };

                    case false: {
                        private _objectId = generateUid;
                        missionNamespace setVariable [_objectId, _object, true];

                        [
                            "if !(missionNamespace getVariable '", _actionExistenceId, "') exitWith {
                                player removeAction _actionId;
                                false;
                            };
                            
                            if (((missionNamespace getVariable '", _objectId, "') distance player) <= ", _distance, ") then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];
                            
                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };

                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            }
                            else {
                                false;
                            }"
                        ] joinString "";
                    };

                    default {
                        [
                            "if !(missionNamespace getVariable '", _actionExistenceId, "') exitWith {
                                player removeAction _actionId;
                                false;
                            };
                            
                            private _viewTarget = call KH_fnc_getViewTarget;

                            if (((_viewTarget select 1) <= ", _distance, ") && (((_viewTarget select 5) param [0, '']) isEqualTo '", _detection, "')) then {
                                private _caller = _this;
                                private _handlerId = [missionNamespace, '", _actionExistenceId, "', true];

                                if ((missionNamespace getVariable '", _drawHintReferenceId, "') isNotEqualTo []) then {
                                    (missionNamespace getVariable '", _drawHintReferenceId, "') call KH_fnc_draw3d;  
                                };

                                (((missionNamespace getVariable '", _argumentsReferenceId, "') call (missionNamespace getVariable '", _conditionShowReferenceId, "')) && (missionNamespace getVariable '", _conditionShowId, "'))
                            }
                            else {
                                false;
                            }"
                        ] joinString "";
                    };
                };
            },
            0,
            _showUnconscious,
            "",
            ""
        ];

        player setUserActionText [_action, _menuName, _windowBackgroundName, _windowForegroundName];

        [
            ["ENTITY", player, "LOCAL"],
            "Respawn",
            [
                _object, 
                _name, 
                _arguments, 
                _function, 
                _condition, 
                _repeatable, 
                _exclusive,
                player,
                false,
                _duration, 
                _distance,
                _showImmediately,
                _showUnconscious,
                _hideOnUse,
                _drawHint, 
                _detection, 
                _userInput, 
                _progressDisplay, 
                _parsedActionId
            ],
            "KH_fnc_addAction"
        ] call KH_fnc_addEventHandler;
    },
    _target,
    true,
    _jip
] call KH_fnc_execute;

[
    [_object, _actionExistenceId],
    {
        params ["_object", "_actionExistenceId"];
        private _deletionHandlerId = ["KH_var_objectActionId", _object, _actionExistenceId] joinString "_";

        if (isNil {_object getVariable _deletionHandlerId;}) then {
            _object setVariable [_deletionHandlerId, true];

            [
                ["ENTITY", _object, "LOCAL"],
                "Deleted",
                [_actionExistenceId],
                {
                    _args params ["_actionExistenceId"];
                    [[missionNamespace, _actionExistenceId, true]] call KH_fnc_removeHandler;
                }
            ] call KH_fnc_addEventHandler;
        };
    },
    "KH_fnc_addEventHandler",
    "SERVER",
    true,
    false
] call KH_fnc_execute;

[missionNamespace, _actionExistenceId, true];