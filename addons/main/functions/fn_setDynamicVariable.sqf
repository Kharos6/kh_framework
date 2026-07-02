params [
    ["_namespace", missionNamespace, [objNull, teamMemberNull, grpNull, locationNull, missionNamespace, taskNull, controlNull, displayNull]], 
    ["_name", "", [""]],
    ["_baseValue", 0, [0, [], createHashMap]],
    ["_changingValue", 1, [0, "", [], createHashMap]],
    ["_minimumValue", 0, [0, [], createHashMap]],
    ["_maximumValue", 1, [0, [], createHashMap]],
    ["_method", "ADD", ["", []]],
    ["_curveType", "", ["", []]],
    "_conditionArguments",
    ["_conditionFunction", {true;}, [{}]],
    ["_interval", 0, [0]],
    ["_target", false, [true, 0, []]]
];

if (_namespace isNil "KH_var_dynamicVariables") then {
    _namespace setVariable ["KH_var_dynamicVariables", createHashMap];
};

if (_namespace isNil _name) then {
    _namespace setVariable [_name, _baseValue];
};

private _entry = (_namespace getVariable ["KH_var_dynamicVariables", createHashMap]) get _name;

private _continue = if !(isNil "_entry") then {
    if ((_entry param [13, 0]) isNotEqualTo _interval) then {
        [[missionNamespace, _entry param [14, ""], clientOwner]] call KH_fnc_removeHandler;
        true;
    }
    else {
        if (_curveType isEqualType "") then {
            if (_curveType isNotEqualTo "") then {
                _curveType = [toUpperANSI _curveType];
            }
            else {
                _curveType = [];
            };
        }
        else {
            if (_curveType isNotEqualTo []) then {
                _curveType set [0, toUpperANSI (_curveType param [0, "", [""]])];
            };
        };

        if (_baseValue isEqualType 0) then {
            _baseValue = [_baseValue];
        };

        if (_changingValue isEqualType 0) then {
            _changingValue = [_changingValue];
        };

        if (_minimumValue isEqualType 0) then {
            _minimumValue = [_minimumValue];
        };

        if (_maximumValue isEqualType 0) then {
            _maximumValue = [_maximumValue];
        };

        KH_var_dynamicVariables set [
            _name, 
            [
                _baseValue, 
                _baseValue isEqualType [], 
                _changingValue, 
                _changingValue isEqualType [], 
                _minimumValue,
                _minimumValue isEqualType [], 
                _maximumValue, 
                _maximumValue isEqualType [], 
                _method, 
                _curveType, 
                _conditionArguments, 
                _conditionFunction, 
                _target, 
                _interval, 
                _entry param [14, ""]
            ]
        ];

        false;
    };
}
else {
    true;
};

if !_continue exitWith {
    [missionNamespace, _entry param [14, ""], clientOwner];
};

private _existenceId = generateUid;
missionNamespace setVariable [_existenceId, true];

if (_curveType isEqualType "") then {
    if (_curveType isNotEqualTo "") then {
        _curveType = [toUpperANSI _curveType];
    }
    else {
        _curveType = [];
    };
}
else {
    if (_curveType isNotEqualTo []) then {
        _curveType set [0, toUpperANSI (_curveType param [0, "", [""]])];
    };
};

if (_baseValue isEqualType 0) then {
    _baseValue = [_baseValue];
};

if (_changingValue isEqualType 0) then {
    _changingValue = [_changingValue];
};

if (_minimumValue isEqualType 0) then {
    _minimumValue = [_minimumValue];
};

if (_maximumValue isEqualType 0) then {
    _maximumValue = [_maximumValue];
};

KH_var_dynamicVariables set [
    _name, 
    [
        _baseValue, 
        _baseValue isEqualType [], 
        _changingValue, 
        _changingValue isEqualType [], 
        _minimumValue, 
        _minimumValue isEqualType [], 
        _maximumValue, 
        _maximumValue isEqualType [], 
        _method, 
        _curveType, 
        _conditionArguments, 
        _conditionFunction, 
        _target, 
        _interval, 
        _existenceId
    ]
];

[
    [_namespace, _name, _existenceId],
    {
        params ["_namespace", "_name"];
        private _entry = KH_var_dynamicVariables get _name;

        if (isNil "_entry") exitWith {
            [_handlerId] call KH_fnc_removeHandler;
        };

        _entry params [
            "_baseValue", 
            "_baseValueIsArray", 
            "_changingValue", 
            "_changingValueIsArray", 
            "_minimumValue", 
            "_minimumValueIsArray", 
            "_maximumValue", 
            "_maximumValueIsArray", 
            "_method", 
            "_curveType", 
            "_args", 
            "_conditionFunction", 
            "_target", 
            "_interval"
        ];
        
        private _variable = _namespace getVariable _name;

        if (isNil "_variable") exitWith {
            KH_var_dynamicVariables deleteAt _name;
            [_handlerId] call KH_fnc_removeHandler;
        };

        private _isArrayOrHashMap = !(_variable isEqualType 0);

        if !_isArrayOrHashMap then {
            _variable = [_variable];
        };

        private _variableIsArray = _variable isEqualType [];
        private _currentBaseValue = 0;
        private _currentChangingValue = 1;
        private _currentMinimumValue = 0;
        private _currentMaximumValue = 1;
        private ["_currentValue", "_currentKey", "_keys"];

        if !_baseValueIsArray then {
            _keys = (keys _baseValue) sort true;
        };

        for "_i" from 0 to (((count _variable) - 1) min ((count _baseValue) - 1)) do {
            if _variableIsArray then {
                _currentBaseValue = _baseValue param [_i, _currentBaseValue];
                _currentChangingValue = _changingValue param [_i, _currentChangingValue];
                _currentMinimumValue = _minimumValue param [_i, _currentMinimumValue];
                _currentMaximumValue = _maximumValue param [_i, _currentMaximumValue];
                _currentValue = _variable param [_i, _currentValue];
                _currentKey = _i;
            }
            else {
                _currentKey = _keys param [_i, _currentKey];
                _currentValue = _variable get _currentKey;

                if (isNil "_currentValue") then {
                    continue;
                };

                if _baseValueIsArray then {
                    _currentBaseValue = _baseValue param [_i, _currentBaseValue];
                }
                else {
                    if !(_baseValue isNil _currentKey) then {
                        _currentBaseValue = _baseValue get _currentKey;
                    };
                };

                if _changingValueIsArray then {
                    _currentChangingValue = _changingValue param [_i, _currentChangingValue];
                }
                else {
                    if !(_changingValue isNil _currentKey) then {
                        _currentChangingValue = _changingValue get _currentKey;
                    };
                };

                if _minimumValueIsArray then {
                    _currentMinimumValue = _minimumValue param [_i, _currentMinimumValue];
                }
                else {
                    if !(_minimumValue isNil _currentKey) then {
                        _currentMinimumValue = _minimumValue get _currentKey;
                    };
                };

                if _maximumValueIsArray then {
                    _currentMaximumValue = _maximumValue param [_i, _currentMaximumValue];
                }
                else {
                    if !(_maximumValue isNil _currentKey) then {
                        _currentMaximumValue = _maximumValue get _currentKey;
                    };
                };
            };

            if ([_namespace, _currentValue, _currentBaseValue, _currentChangingValue, _currentMinimumValue, _currentMaximumValue, _interval] call _conditionFunction) then {
                if (_currentChangingValue isEqualType "") then {
                    _currentChangingValue = (parseNumber _currentChangingValue) * _totalDelta;
                };
            
                _variable set [
                    _currentKey,
                    if (_curveType isEqualTo []) then {
                        switch _method do {
                            case "ADD": {
                                ((_currentValue + _currentChangingValue) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "SUBTRACT": {
                                ((_currentValue - _currentChangingValue) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "MULTIPLY": {
                                ((_currentValue * _currentChangingValue) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "DIVIDE": {
                                ((_currentValue / _currentChangingValue) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "SET": {
                                (_currentChangingValue min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "RESET": {
                                (_currentBaseValue min _currentMaximumValue) max _currentMinimumValue;
                            };

                            default {
                                _method params ["_args", ["_function", {}, [{}]]];
                                (([_namespace, _currentValue, _currentBaseValue, _currentChangingValue, _currentMinimumValue, _currentMaximumValue, _interval] call _function) min _currentMaximumValue) max _currentMinimumValue;  
                            };
                        };
                    }
                    else {
                        _curveType params [["_curve", "LINEAR", [""]], ["_floor", 0.01, [0]], ["_curvePoints", [], [[]]]];

                        switch _method do {
                            case "ADD": {
                                ((_currentValue + (_currentChangingValue * (_curve curveSlope ([[_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _floor], [_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _curvePoints, _floor]] select (_curve isEqualTo "BEZIER"))))) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "SUBTRACT": {
                                ((_currentValue - (_currentChangingValue * (_curve curveSlope ([[_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _floor], [_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _curvePoints, _floor]] select (_curve isEqualTo "BEZIER"))))) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "MULTIPLY": {
                                ((_currentValue * (_currentChangingValue * (_curve curveSlope ([[_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _floor], [_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _curvePoints, _floor]] select (_curve isEqualTo "BEZIER"))))) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "DIVIDE": {
                                ((_currentValue / (_currentChangingValue * (_curve curveSlope ([[_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _floor], [_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _curvePoints, _floor]] select (_curve isEqualTo "BEZIER"))))) min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "SET": {
                                (_currentChangingValue min _currentMaximumValue) max _currentMinimumValue;
                            };

                            case "RESET": {
                                (_currentBaseValue min _currentMaximumValue) max _currentMinimumValue;
                            };

                            default {
                                _method params ["_args", ["_function", {}, [{}]]];

                                (
                                    (
                                        [
                                            _namespace, 
                                            _currentValue,
                                            _currentBaseValue, 
                                            _currentChangingValue * (_curve curveSlope ([[_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _floor], [_currentMinimumValue, _currentMaximumValue, _currentValue, 0, 1, _curvePoints, _floor]] select (_curve isEqualTo "BEZIER"))), 
                                            _currentMinimumValue, 
                                            _currentMaximumValue, 
                                            _interval
                                        ] call _function
                                    ) 
                                min _currentMaximumValue) max _currentMinimumValue;
                            };
                        };
                    }
                ];
            }
            else {
                _variable set [_currentKey, _currentValue];
            };
        };

        if !_isArrayOrHashMap then {        
            _namespace setVariable [_name, _variable param [0, 0], _target];
        }
        else {
            _namespace setVariable [_name, _variable, _target];
        };
    },
    true,
    [
        {
            private _existenceId = param [2];
            missionNamespace getVariable _existenceId;
        },
        false,
        _interval,
        [
            0,
            false,
            true,
            true
        ],
        {
            private _name = param [1];
            KH_var_dynamicVariables deleteAt _name;
        },
        _interval isNotEqualTo 0,
        true
    ],
    false
] call KH_fnc_execute;

[missionNamespace, _existenceId, clientOwner];