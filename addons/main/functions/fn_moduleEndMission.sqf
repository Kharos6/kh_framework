isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			_logic getVariable ["KH_ModuleEndMissionEndName", ""], 
			_logic getVariable ["KH_ModuleEndMissionVictory", true], 
			parseNumber (_logic getVariable ["KH_ModuleEndMissionFadeType", "0"]),
			parseNumber (_logic getVariable ["KH_ModuleEndMissionDelay", "1"]),
			parseSimpleArray (_logic getVariable ["KH_ModuleEndMissionNextMission", "[]"])
		] call KH_fnc_endMission;
	};
};

true;