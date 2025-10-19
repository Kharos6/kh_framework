isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
		private _parsedNextMission = parseSimpleArray (["[", _logic getVariable ["KH_ModuleEndMissionNextMission", ""], "]"] joinString "");

		[
			_logic getVariable ["KH_ModuleEndMissionEndName", "KH_MissionConcluded"], 
			_logic getVariable ["KH_ModuleEndMissionVictory", true], 
			parseNumber (_logic getVariable ["KH_ModuleEndMissionFadeType", "0"]),
			parseNumber (_logic getVariable ["KH_ModuleEndMissionDelay", "1"]),
			_parsedNextMission
		] call KH_fnc_endMission;
	};
};

nil;