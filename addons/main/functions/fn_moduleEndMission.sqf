isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
		private _parsedNextMission = parseSimpleArray (["[", _logic getVariable ["KH_ModuleEndMissionNextMission", ""], "]"] joinString "");
		private _fadeType = parseNumber (_logic getVariable ["KH_ModuleEndMissionFadeType", "-2"]);

		if (_fadeType isEqualTo -2) then {
			_fadeType = true;
		}
		else {
			if (_fadeType isEqualTo -1) then {
				_fadeType = false;
			};
		};

		[
			_logic getVariable ["KH_ModuleEndMissionName", "KH_MissionConcluded"], 
			_logic getVariable ["KH_ModuleEndMissionVictory", true], 
			_fadeType,
			parseNumber (_logic getVariable ["KH_ModuleEndMissionDelay", "1"]),
			_parsedNextMission
		] call KH_fnc_endMission;
	};
};

nil;