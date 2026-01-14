isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        [_logic getVariable ["KH_ModulePlayRecordedMissionIdentifier", ""]] call KH_fnc_playRecordedMission;
    };
};

nil;