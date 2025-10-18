params [["_event", "", [""]], "_arguments", ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], ["_jip", false, [true, []]]];

private _return = switch (typeName _target) do {
    case "BOOL": {
        if _target then {
            [_event, _arguments] call CBA_fnc_localEvent;
        };
    };

    case "SCALAR": {
        if (_target isEqualTo clientOwner) then {
            [_event, _arguments] call CBA_fnc_localEvent;
        }
        else {
            if (_target >= 0) then {
                [_event, _arguments, _target] call CBA_fnc_ownerEvent;
            }
            else {
                {
                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                } forEach (KH_var_allMachines - [abs _target]);
            };
        };
    };

    case "OBJECT": {
        if (local _target) then {
            [_event, _arguments] call CBA_fnc_localEvent;
        }
        else {
            [_event, _arguments, _target] call CBA_fnc_targetEvent;
        };
    };

    case "TEAM_MEMBER": {
        if (local (agent _target)) then {
            [_event, _arguments] call CBA_fnc_localEvent;
        }
        else {
            [_event, _arguments, agent _target] call CBA_fnc_targetEvent;
        };
    };

    case "GROUP": {
        private _playerUnits = KH_var_allPlayerUnits select {(group _x) isEqualTo _target;};

        if (((local _target) && (_playerUnits isEqualTo [])) || ((local _target) && (player in _playerUnits) && ((count _playerUnits) <= 1))) then {
            [_event, _arguments] call CBA_fnc_localEvent;
        }
        else {
            if (_playerUnits isEqualTo []) then {
                [_event, _arguments, _target] call CBA_fnc_targetEvent;
            }   
            else {
                [
                    [_event, _arguments, _target],
                    {
                        params ["_event", "_arguments", "_target"];
                        private _targets = [];

                        {
                            _targets pushBackUnique (owner _x);
                        } forEach (units _target);

                        {
                            [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                        } forEach _targets;
                    },
                    "SERVER",
                    true,
                    false
                ] call KH_fnc_execute;
            };
        };
    };

    case "SIDE": {
        [_event, _arguments, KH_var_allPlayerUnits select {(side (group _x)) isEqualTo _target;}] call CBA_fnc_targetEvent;
    };

    case "STRING": {
        switch _target do {
            case "LOCAL": {
                [_event, _arguments] call CBA_fnc_localEvent;
            };
            
            case "SERVER": {
                if isServer then {
                    [_event, _arguments] call CBA_fnc_localEvent;
                }
                else {
                    [_event, _arguments] call CBA_fnc_serverEvent;
                };
            };

            case "GLOBAL": {
                [_event, _arguments] call CBA_fnc_globalEvent;
            };

            case "REMOTE": {
                [_event, _arguments] call CBA_fnc_remoteEvent;
            };

            case "PLAYERS": {
                {
                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                } forEach KH_var_allPlayerMachines;
            };

            case "ADMIN": {
                if (KH_var_adminMachine isEqualTo clientOwner) then {
                    [_event, _arguments] call CBA_fnc_localEvent;
                }
                else {
                    [_event, _arguments, KH_var_adminMachine] call CBA_fnc_ownerEvent;
                };
            };

            case "CURATORS": {
                [_event, _arguments, KH_var_allPlayerUnits select {!(isNull (getAssignedCuratorLogic _x));}] call CBA_fnc_targetEvent;
            };

            case "HEADLESS": {
                {
                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                } forEach KH_var_allHeadlessMachines;
            };

            default {
                if ((parseNumber (_target select [0, 1])) isNotEqualTo 0) then {
                    if !(":" in _target) then {
                        private _client = KH_var_allPlayerUidMachines get _target;

                        if !(isNil "_client") then {
                            if (_client isEqualTo clientOwner) then {
                                [_event, _arguments] call CBA_fnc_localEvent;
                            }
                            else {
                                [_event, _arguments, _client] call CBA_fnc_ownerEvent;
                            };
                        }
                        else {
                            _client = KH_var_allPlayerIdMachines get _target;

                            if !(isNil "_client") then {
                                if (_client isEqualTo clientOwner) then {
                                    [_event, _arguments] call CBA_fnc_localEvent;
                                }
                                else {
                                    [_event, _arguments, _client] call CBA_fnc_ownerEvent;
                                };
                            }
                            else {
                                _client = KH_var_allHeadlessIdMachines get _target;

                                if !(isNil "_client") then {
                                    if (_client isEqualTo clientOwner) then {
                                        [_event, _arguments] call CBA_fnc_localEvent;
                                    }
                                    else {
                                        [_event, _arguments, _client] call CBA_fnc_ownerEvent;
                                    };
                                }
                                else {
                                    private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                                    if (_playerTargets isNotEqualTo []) then {
                                        [_event, _arguments, _playerTargets] call CBA_fnc_targetEvent;
                                    }
                                    else {
                                        private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                                        if (_groupTargets isNotEqualTo []) then {
                                            {
                                                private _target = _group;
                                                private _playerUnits = KH_var_allPlayerUnits select {(group _x) isEqualTo _target;};

                                                if (((local _target) && (_playerUnits isEqualTo [])) || ((local _target) && (player in _playerUnits) && ((count _playerUnits) <= 1))) then {
                                                    [_event, _arguments] call CBA_fnc_localEvent;
                                                }
                                                else {
                                                    if (_playerUnits isEqualTo []) then {
                                                        [_event, _arguments, _target] call CBA_fnc_targetEvent;
                                                    }   
                                                    else {
                                                        [
                                                            [_event, _arguments, _target],
                                                            {
                                                                params ["_event", "_arguments", "_target"];
                                                                private _targets = [];

                                                                {
                                                                    _targets pushBackUnique (owner _x);
                                                                } forEach (units _target);

                                                                {
                                                                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                                                                } forEach _targets;
                                                            },
                                                            "SERVER",
                                                            true,
                                                            false
                                                        ] call KH_fnc_execute;
                                                    };
                                                };
                                            } forEach _groupTargets;
                                        }
                                        else {
                                            if (_target in allMapMarkers) then {
                                                [_event, _arguments, KH_var_allPlayerUnits select {_x inArea _target;}] call CBA_fnc_targetEvent;
                                            };
                                        };
                                    };
                                };
                            };
                        };
                    }
                    else {
                        if !(isNull (objectFromNetId _target)) then {
                            if (local (objectFromNetId _target)) then {
                                [_event, _arguments] call CBA_fnc_localEvent;
                            }
                            else {
                                [_event, _arguments, objectFromNetId _target] call CBA_fnc_targetEvent;
                            };
                        }
                        else {
                            if !(isNull (groupFromNetId _target)) then {
                                _target = groupFromNetId _target;
                                private _playerUnits = KH_var_allPlayerUnits select {(group _x) isEqualTo _target;};

                                if (((local _target) && (_playerUnits isEqualTo [])) || ((local _target) && (player in _playerUnits) && ((count _playerUnits) <= 1))) then {
                                    [_event, _arguments] call CBA_fnc_localEvent;
                                }
                                else {
                                    if (_playerUnits isEqualTo []) then {
                                        [_event, _arguments, _target] call CBA_fnc_targetEvent;
                                    }   
                                    else {
                                        [
                                            [_event, _arguments, _target],
                                            {
                                                params ["_event", "_arguments", "_target"];
                                                private _targets = [];

                                                {
                                                    _targets pushBackUnique (owner _x);
                                                } forEach (units _target);

                                                {
                                                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                                                } forEach _targets;
                                            },
                                            "SERVER",
                                            true,
                                            false
                                        ] call KH_fnc_execute;
                                    };
                                };
                            }
                            else {
                                private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                                if (_playerTargets isNotEqualTo []) then {
                                    [_event, _arguments, _playerTargets] call CBA_fnc_targetEvent;
                                }
                                else {
                                    private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                                    if (_groupTargets isNotEqualTo []) then {
                                        {
                                            private _target = _group;
                                            private _playerUnits = KH_var_allPlayerUnits select {(group _x) isEqualTo _target;};

                                            if (((local _target) && (_playerUnits isEqualTo [])) || ((local _target) && (player in _playerUnits) && ((count _playerUnits) <= 1))) then {
                                                [_event, _arguments] call CBA_fnc_localEvent;
                                            }
                                            else {
                                                if (_playerUnits isEqualTo []) then {
                                                    [_event, _arguments, _target] call CBA_fnc_targetEvent;
                                                }   
                                                else {
                                                    [
                                                        [_event, _arguments, _target],
                                                        {
                                                            params ["_event", "_arguments", "_target"];
                                                            private _targets = [];

                                                            {
                                                                _targets pushBackUnique (owner _x);
                                                            } forEach (units _target);

                                                            {
                                                                [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                                                            } forEach _targets;
                                                        },
                                                        "SERVER",
                                                        true,
                                                        false
                                                    ] call KH_fnc_execute;
                                                };
                                            };
                                        } forEach _groupTargets;
                                    }
                                    else {
                                        if (_target in allMapMarkers) then {
                                            [_event, _arguments, KH_var_allPlayerUnits select {_x inArea _target;}] call CBA_fnc_targetEvent;
                                        };
                                    };
                                };
                            };							
                        };
                    };
                }
                else {
                    private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                    if (_playerTargets isNotEqualTo []) then {
                        [_event, _arguments, _playerTargets] call CBA_fnc_targetEvent;
                    }
                    else {
                        private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                        if (_groupTargets isNotEqualTo []) then {
                            {
                                private _target = _group;
                                private _playerUnits = KH_var_allPlayerUnits select {(group _x) isEqualTo _target;};

                                if (((local _target) && (_playerUnits isEqualTo [])) || ((local _target) && (player in _playerUnits) && ((count _playerUnits) <= 1))) then {
                                    [_event, _arguments] call CBA_fnc_localEvent;
                                }
                                else {
                                    if (_playerUnits isEqualTo []) then {
                                        [_event, _arguments, _target] call CBA_fnc_targetEvent;
                                    }   
                                    else {
                                        [
                                            [_event, _arguments, _target],
                                            {
                                                params ["_event", "_arguments", "_target"];
                                                private _targets = [];

                                                {
                                                    _targets pushBackUnique (owner _x);
                                                } forEach (units _target);

                                                {
                                                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                                                } forEach _targets;
                                            },
                                            "SERVER",
                                            true,
                                            false
                                        ] call KH_fnc_execute;
                                    };
                                };
                            } forEach _groupTargets;
                        }
                        else {
                            if (_target in allMapMarkers) then {
                                [_event, _arguments, KH_var_allPlayerUnits select {_x inArea _target;}] call CBA_fnc_targetEvent;
                            };
                        };
                    };
                };
            };
        };
    };

    case "ARRAY": {
        [
            [_event, _arguments, flatten _target, clientOwner],
            {
                params ["_event", "_arguments", "_targets", "_caller"];
                private _parsedTargets = [];

                {
                    private _target = _targets param [_forEachIndex, nil, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]];

                    if (isNil "_target") then {
                        continue;
                    };

                    switch (typeName _target) do {
                        case "BOOL": {
                            if _target then {
                                _parsedTargets pushBackUnique _caller;
                            };
                        };

                        case "SCALAR": {
                            _parsedTargets pushBackUnique _target;
                        };

                        case "OBJECT": {
                            _parsedTargets pushBackUnique (owner _target);
                        };

                        case "TEAM_MEMBER": {
                            _parsedTargets pushBackUnique (owner (agent _target));
                        };

                        case "GROUP": {
                            {
                                _parsedTargets pushBackUnique (owner _x);
                            } forEach (units _target);
                        };

                        case "SIDE": {
                            _parsedTargets insert [-1, (KH_var_allPlayerUnits select {(side (group _x)) isEqualTo _target;}), true];
                        };
                        
                        case "STRING": {
                            switch _target do {
                                case "LOCAL": {
                                    _parsedTargets pushBackUnique _caller;
                                };

                                case "SERVER": {
                                    _parsedTargets pushBackUnique 2;
                                };

                                case "GLOBAL": {
                                    _parsedTargets insert [-1, KH_var_allMachines, true];
                                };

                                case "REMOTE": {
                                    _parsedTargets insert [-1, KH_var_allMachines - [_caller], true];
                                };

                                case "PLAYERS": {
                                    _parsedTargets insert [-1, KH_var_allPlayerMachines, true];
                                };

                                case "ADMIN": {
                                    _parsedTargets pushBackUnique KH_var_adminMachine;
                                };

                                case "CURATORS": {
                                    _parsedTargets insert [-1, KH_var_allPlayerUnits select {!(isNull (getAssignedCuratorLogic _x));}, true];
                                };

                                case "HEADLESS": {
                                    _parsedTargets insert [-1, KH_var_allHeadlessMachines, true];
                                };

                                default {
                                    if ((parseNumber (_target select [0, 1])) isNotEqualTo 0) then {
                                        if !(":" in _target) then {
                                            private _client = KH_var_allPlayerUidMachines get _target;

                                            if !(isNil "_client") then {
                                                _parsedTargets pushBackUnique _client;
                                            }
                                            else {
                                                _client = KH_var_allPlayerIdMachines get _target;

                                                if !(isNil "_client") then {
                                                    _parsedTargets pushBackUnique _client;
                                                }
                                                else {
                                                    _client = KH_var_allHeadlessIdMachines get _target;

                                                    if !(isNil "_client") then {
                                                        _parsedTargets pushBackUnique _client;
                                                    }
                                                    else {
                                                        private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                                                        if (_playerTargets isNotEqualTo []) then {
                                                            _parsedTargets insert [-1, _playerTargets, true];
                                                        }
                                                        else {
                                                            private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                                                            if (_groupTargets isNotEqualTo []) then {
                                                                {
                                                                    private _target = _x;

                                                                    {
                                                                        _parsedTargets pushBackUnique (owner _x);
                                                                    } forEach (units _target);
                                                                } forEach _groupTargets;
                                                            }
                                                            else {
                                                                if (_target in allMapMarkers) then {
                                                                    _parsedTargets insert [-1, KH_var_allPlayerUnits select {_x inArea _target;}, true];
                                                                };
                                                            };
                                                        };
                                                    };
                                                };
                                            };
                                        }
                                        else {
                                            if !(isNull (objectFromNetId _target)) then {
                                                _parsedTargets pushBackUnique (owner (objectFromNetId _target));
                                            }
                                            else {
                                                if !(isNull (groupFromNetId _target)) then {
                                                    {
                                                        _parsedTargets pushBackUnique (owner _x);
                                                    } forEach (units (groupFromNetId _target));
                                                }
                                                else {
                                                    private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                                                    if (_playerTargets isNotEqualTo []) then {
                                                        _parsedTargets insert [-1, _playerTargets, true];
                                                    }
                                                    else {
                                                        private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                                                        if (_groupTargets isNotEqualTo []) then {
                                                            {
                                                                private _target = _x;

                                                                {
                                                                    _parsedTargets pushBackUnique (owner _x);
                                                                } forEach (units _target);
                                                            } forEach _groupTargets;
                                                        }
                                                        else {
                                                            if (_target in allMapMarkers) then {
                                                                _parsedTargets insert [-1, KH_var_allPlayerUnits select {_x inArea _target;}, true];
                                                            };
                                                        };
                                                    };
                                                };							
                                            };
                                        };
                                    }
                                    else {
                                        private _playerTargets = KH_var_allPlayerUnits select {(((name _x) isEqualTo _target) || ((roleDescription _x) isEqualTo _target));};

                                        if (_playerTargets isNotEqualTo []) then {
                                            _parsedTargets insert [-1, _playerTargets, true];
                                        }
                                        else {
                                            private _groupTargets = allGroups select {(groupId _x) isEqualTo _target;};

                                            if (_groupTargets isNotEqualTo []) then {
                                                {
                                                    private _target = _x;

                                                    {
                                                        _parsedTargets pushBackUnique (owner _x);
                                                    } forEach (units _target);
                                                } forEach _groupTargets;
                                            }
                                            else {
                                                if (_target in allMapMarkers) then {
                                                    _parsedTargets insert [-1, KH_var_allPlayerUnits select {_x inArea _target;}, true];
                                                };
                                            };
                                        };
                                    };
                                };
                            };
                        };

                        case "CODE": {
                            [
                                [_event, _arguments, _parsedTargets],
                                {
                                    params ["_event", "_arguments", "_parsedTargets"];
                                    _argsCallback params ["_eventReceiver"];

                                    if !(isNil "_eventReceiver") then {
                                        if !(_eventReceiver in _parsedTargets) then {
                                            _parsedTargets pushBackUnique _eventReceiver;
                                            [_event, _arguments, _eventReceiver] call CBA_fnc_ownerEvent;
                                        };
                                    };                                      
                                },
                                KH_var_allMachines,
                                true,
                                [
                                    "CALLBACK",
                                    [_arguments, _target],
                                    {
                                        params [["_arguments", []], "_function"];

                                        if (_arguments call _function) then {
                                            [clientOwner];
                                        }
                                        else {
                                            [];
                                        };						
                                    }
                                ]
                            ] call KH_fnc_execute;
                        };

                        case "LOCATION": {
                            _parsedTargets insert [-1, KH_var_allPlayerUnits select {_x inArea _target;}, true];
                        };
                    };
                } forEach _targets;

                {
                    [_event, _arguments, _x] call CBA_fnc_ownerEvent;
                } forEach _parsedTargets;
            },
            "SERVER",
            true,
            false
        ] call KH_fnc_execute;
    };

    case "CODE": {
        [
            [_event, _arguments, _target],
            {
                params ["_event", ["_arguments", []], "_function"];

                if (_arguments call _function) then {
                    [_event, _arguments] call CBA_fnc_localEvent;
                };
            },
            "GLOBAL",
            true,
            false
        ] call KH_fnc_execute;
    };

    case "LOCATION": {
        [_event, _arguments, KH_var_allPlayerUnits select {_x inArea _target;}] call CBA_fnc_targetEvent;
    };
    
    default {
        nil;
    };
};

if (_jip isNotEqualTo false) exitWith {
    if (_jip isEqualTo true) then {
        private _jipId = generateUid;
        ["KH_eve_jipSetup", [_event, _arguments, true, false, _jipId]] call CBA_fnc_serverEvent;
        [missionNamespace, _jipId, 2];
    }
    else {
        _jip params [
            ["_dependency", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull]], 
            ["_unitRequired", false, [true]], 
            ["_jipId", "", [""]]
        ];

        if (_jipId isEqualTo "") then {
            _jipId = generateUid;
        };

        ["KH_eve_jipSetup", [_event, _arguments, _dependency, _unitRequired, _jipId]] call CBA_fnc_serverEvent;
        [missionNamespace, _jipId, 2];
    };
};

_return;