isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        private _owner = _logic getVariable ["KH_ModuleRecordMissionCollector", ""];
        private _identifier = _logic getVariable ["KH_ModuleRecordMissionIdentifier", ""];
        private _units = _logic getVariable ["KH_ModuleRecordMissionUnits", true];
        private _groups = _logic getVariable ["KH_ModuleRecordMissionGroups", true];
        private _objects = _logic getVariable ["KH_ModuleRecordMissionObjects", true];
        private _scenario = _logic getVariable ["KH_ModuleRecordMissionScenario", true];
        
        if (_owner isEqualTo "") then {
            [_identifier, _units, _groups, _objects, _scenario] call KH_fnc_recordMission;
        }
        else {
            [
                [_owner, [_identifier, _units, _groups, _objects, _scenario]],
                {
                    private _arguments = param [1];
                    _arguments call KH_fnc_recordMission;
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
    };
};

nil;