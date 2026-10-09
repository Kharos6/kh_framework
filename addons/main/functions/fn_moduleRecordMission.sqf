isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        private _owner = _logic getVariable ["KH_ModuleRecordMissionCollector", ""];
        private _identifier = _logic getVariable ["KH_ModuleRecordMissionIdentifier", ""];
        private _units = _logic getVariable ["KH_ModuleRecordMissionUnits", true];
        private _groups = _logic getVariable ["KH_ModuleRecordMissionGroups", true];
        private _objects = _logic getVariable ["KH_ModuleRecordMissionObjects", true];
        private _scenario = _logic getVariable ["KH_ModuleRecordMissionScenario", true];
        private _captureFinalData = _logic getVariable ["KH_ModuleRecordMissionCaptureFinalData", true];
        
        if (_owner isEqualTo "") then {
            [_identifier, _units, _groups, _objects, _scenario, _captureFinalData] call KH_fnc_recordMission;
        }
        else {
            execute [
                [_owner, [_identifier, _units, _groups, _objects, _scenario, _captureFinalData]],
                {
                    params ["_owner", "_arguments"];
                    execute [_arguments, "KH_fnc_recordMission", missionNamespace getVariable _owner, true, false];
                },
                true,
                {
                    params ["_owner"];
                    
                    if !(missionNamespace isNil _owner) then {
                        private _unit = missionNamespace getVariable _owner;
                        ((!(isNull _unit) && !(local _unit)) || (isPlayer _unit));
                    }
                    else {
                        false;
                    };
                },
                false
            ];
        };
    };
};

nil;