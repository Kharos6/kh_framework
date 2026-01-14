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
_data set ["UnitJoined", []];
_data set ["UnitLeft", []];

private _result = [
    _group getVariable ["KH_var_allCollectedData", []],
    _data,
    [
        [
            "MISSION",
            "GroupDeleted",
            [_group, _data],
            {
                params ["_deletedGroup"];
                _args params ["_group", "_data"];

                if (_group isEqualTo _deletedGroup) then {
                    _data set ["Deleted", CBA_missionTime];
                    [_handlerId] call KH_fnc_removeHandler;
                };
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
        ] call KH_fnc_addEventHandler
    ]
];

_group setVariable ["KH_var_dataCollectionHandlers", _result select 2];
_result;