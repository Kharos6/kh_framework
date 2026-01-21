params [["_identifier", "", [""]], ["_units", true, [true]], ["_groups", true, [true]], ["_objects", true, [true]], ["_scenario", true, [true]], ["_captureFinalData", true, [true]]];
KH_var_recordedUnitData = createHashMap;
KH_var_recordedGroupData = createHashMap;
KH_var_recordedObjectData = createHashMap;
KH_var_recordedScenarioData = [];

if _units then {
    [
        ["Man"], 
        ["Animal"],
        {
            params ["_unit"];

            [
                [_unit],
                {
                    params ["_unit"];

                    if (_unit getVariable ["KH_var_allowDataCollection", true]) then {
                        KH_var_recordedUnitData set [netId _unit, ([_unit, false] call KH_fnc_collectUnitData) select 0];
                    };
                },
                true,
                "-2",
                false
            ] call KH_fnc_execute;
        }, 
        true
    ] call KH_fnc_entityInit;
};

if _groups then {
    {
        [
            [_x],
            {
                params ["_group"];

                if (_group getVariable ["KH_var_allowDataCollection", true]) then {
                    KH_var_recordedGroupData set [netId _group, ([_group] call KH_fnc_collectGroupData) select 0];
                };
            },
            true,
            "-2",
            false
        ] call KH_fnc_execute;
    } forEach allGroups;
};

if _objects then {
    [
        "MISSION",
        "GroupCreated",
        [],
        {
            params ["_group"];

            [
                [_group],
                {
                    params ["_group"];

                    if (_group getVariable ["KH_var_allowDataCollection", true]) then {
                        KH_var_recordedGroupData set [netId _group, ([_group] call KH_fnc_collectGroupData) select 0];
                    };
                },
                true,
                "-2",
                false
            ] call KH_fnc_execute;
        }
    ] call KH_fnc_addEventHandler;

    [
        [], 
        ["Man", "Logic", "EmptyDetector"],
        {
            params ["_object"];

            [
                [_object],
                {
                    params ["_object"];

                    if (_object getVariable ["KH_var_allowDataCollection", true]) then {
                        KH_var_recordedObjectData set [netId _object, ([_object] call KH_fnc_collectObjectData) select 0];
                    };
                },
                true,
                "-2",
                false
            ] call KH_fnc_execute;
        }, 
        true
    ] call KH_fnc_entityInit;
};

if _scenario then {
    [
        [],
        {
            KH_var_recordedScenarioData = (call KH_fnc_collectScenarioData) select 0;
        },
        true,
        "-2",
        false
    ] call KH_fnc_execute;
};

[
    "CBA",
    "KH_eve_missionEnded",
    [_identifier, _captureFinalData],
    {
        _args params ["_identifier", "_captureFinalData"];
        private _unitDataFinal = createHashMap;
        private _groupDataFinal = createHashMap;
        private _objectDataFinal = createHashMap;
        private _scenarioDataFinal = [];
        _identifier writeKhData ["sessionId", KH_var_serverMissionSessionId];
        _identifier writeKhData ["unitData", KH_var_recordedUnitData];
        _identifier writeKhData ["groupData", KH_var_recordedGroupData];
        _identifier writeKhData ["objectData", KH_var_recordedObjectData];
        _identifier writeKhData ["scenarioData", KH_var_recordedScenarioData];

        if _captureFinalData then { 
            if _units then {
                {
                    _unitDataFinal set [netId _x, [_x] call KH_fnc_getUnitAttributes];
                } forEach (allUnits + allDeadMen);
            };

            if _groups then {
                {
                    _groupDataFinal set [netId _x, [_x] call KH_fnc_getGroupAttributes];
                } forEach allGroups;
            };

            if _objects then {
                {
                    _objectDataFinal set [netId _x, [_x] call KH_fnc_getObjectAttributes];
                } forEach (entities [[], ["Man"], false, true]);
            };

            if _scenario then {
                _unitDataFinal = call KH_fnc_getMissionAttributes;
            };
        };

        _identifier writeKhData ["unitDataFinal", _unitDataFinal];
        _identifier writeKhData ["groupDataFinal", _groupDataFinal];
        _identifier writeKhData ["objectDataFinal", _objectDataFinal];
        _identifier writeKhData ["scenarioDataFinal", _scenarioDataFinal];
        _identifier writeKhData ["dataEnd", CBA_missionTime];
    }
] call KH_fnc_addEventHandler;

nil;