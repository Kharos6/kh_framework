params [["_group", grpNull, [grpNull]]];
private _name = ["KH_var_collectedData_", CBA_missionTime] joinString "";
_group setVariable [_name, createHashMap];
private _data = _group getVariable _name;

if !(_group getVariable ["KH_var_allowDataCollection", true]) exitWith {
    [
        _group getVariable ["KH_var_allCollectedData", []],
        _data,
        []
    ];
};

if ((_group getVariable ["KH_var_dataCollectionHandlers", []]) isNotEqualTo []) then {
    [_group getVariable ["KH_var_dataCollectionHandlers", []]] call KH_fnc_removeHandler;
    _group setVariable ["KH_var_dataCollectionHandlers", []];
};

if (_group isNil "KH_var_allCollectedData") then {
    _group setVariable ["KH_var_allCollectedData", []];
};

(_group getVariable ["KH_var_allCollectedData", []]) pushBack [CBA_missionTime, _data];
_data set ["Deleted", -1];
_data set ["GroupID", []];

_data set [
    "Header", 
    [
        netId _group,
        [_group] call KH_fnc_getGroupAttributes
    ]
];

_data set ["Leader", []];
_data set ["Task", []];
_data set ["UnitJoined", []];
_data set ["UnitLeft", []];

private _result = [
    _group getVariable ["KH_var_allCollectedData", []],
    _data,
    [
        [
            ["ENTITY", _group, "LOCAL"],
            "Deleted",
            [_data],
            {
                params ["_group"];
                _args params ["_data"];
                _data set ["Deleted", CBA_missionTime];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _group, "REMOTE"],
            "GroupIdChanged",
            [_data],
            {
                params ["_group", "_newGroupId"];
                _args params ["_data"];
                (_data get "GroupID") pushBack [(CBA_missionTime - ([_group, false] call KH_fnc_getLatency)) max 0, _newGroupId];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _group, "REMOTE"],
            "LeaderChanged",
            [_data],
            {
                params ["_group", "_newLeader"];
                _args params ["_data"];
                (_data get "Leader") pushBack [(CBA_missionTime - ([_group, false] call KH_fnc_getLatency)) max 0, netId _newLeader];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _group, "REMOTE"],
            "UnitJoined",
            [_data],
            {
                params ["_group", "_unit"];
                _args params ["_data"];
                (_data get "UnitJoined") pushBack [(CBA_missionTime - ([_group, false] call KH_fnc_getLatency)) max 0, netId _unit];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _group, "REMOTE"],
            "UnitLeft",
            [_data],
            {
                params ["_group", "_unit"];
                _args params ["_data"];
                (_data get "UnitLeft") pushBack [(CBA_missionTime - ([_group, false] call KH_fnc_getLatency)) max 0, netId _unit];
            }
        ] call KH_fnc_addEventHandler,
        [
            [_group, _data],
            {
                params ["_group", "_data"];

                private _currentTask = if ((count (waypoints _group)) > 1) then {
                    private _currentWaypoint = ((waypoints _group) select {(_x select 1) isEqualTo (currentWaypoint _group);}) select 0;
                    private _taskPosition = getWPPos [_group, _currentWaypoint select 1];

                    private _roadwayTaskIntersection = [
                        _taskPosition vectorAdd [0, 0, 0.1],
                        _taskPosition vectorAdd [0, 0, -0.1],
                        ["TERRAIN"],
                        true,
                        1,
                        "ROADWAY",
                        "NONE",
                        true,
                        []
                    ] call KH_fnc_raycast;

                    private _object = objNull;

                    private _entries = if (_roadwayTaskIntersection isNotEqualTo []) then {
                        _object = _roadwayTaskIntersection select 3;
                        private _currentEntries = [];
                        
                        for "_i" from 0 to 999 do {
                            private _entry = _object buildingExit _i;

                            if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                _currentEntries pushBack _entry;
                            }
                            else {
                                break;
                            };
                        };

                        _currentEntries;
                    }
                    else {
                        _object = waypointAttachedVehicle [_group, currentWaypoint _group];

                        if (isNull _object) then {
                            _object = waypointAttachedObject [_group, currentWaypoint _group];

                            if (isNull _object) then {
                                [];
                            }
                            else {
                                private _currentEntries = [];
                                
                                for "_i" from 0 to 999 do {
                                    private _entry = _object buildingExit _i;

                                    if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                        _currentEntries pushBack _entry;
                                    }
                                    else {
                                        break;
                                    };
                                };

                                _currentEntries;
                            };
                        }
                        else {
                            private _currentEntries = [];
                            
                            for "_i" from 0 to 999 do {
                                private _entry = _object buildingExit _i;

                                if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                    _currentEntries pushBack _entry;
                                }
                                else {
                                    break;
                                };
                            };

                            _currentEntries;
                        };
                    };

                    [
                        waypointType _currentWaypoint, 
                        _taskPosition,
                        netId _object, 
                        if !(isNull _object) then {
                            _object call BIS_fnc_buildingPositions;
                        }
                        else {
                            [];
                        }, 
                        _entries
                    ];
                }
                else {
                    [];
                };

                private _otherTasks = (waypoints _group) apply {[waypointType _x, getWPPos [_group, _x select 1]];};

                if ((_group getVariable ["KH_var_collectedTask", []]) isNotEqualTo [_currentTask, _otherTasks]) then {
                    (_data get "Task") pushBack [_trueTime, [_currentTask, _otherTasks]];
                    _group setVariable ["KH_var_collectedTask", [_currentTask, _otherTasks]];
                };
            },
            true,
            0.1,
            false
        ] call KH_fnc_execute
    ]
];

_group setVariable ["KH_var_dataCollectionHandlers", _result select 2];
_result;