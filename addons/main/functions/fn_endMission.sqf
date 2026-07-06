params [["_endName", "KH_MissionConcluded", [""]], ["_isVictory", true, [true]], ["_fadeType", true, [true, 0]], ["_delay", 1, [0]], ["_nextMission", [], [[]]]];
_delay = _delay max 1;

{
	call _x;
} forEach KH_var_serverMissionEndStack;

call KH_fnc_serverMissionEndInit;

execute [
	[],
	{
		{
			call _x;
		} forEach KH_var_playerMissionEndStack;

		call KH_fnc_playerMissionEndInit;
	},
	"PLAYERS",
	true,
	false
];

execute [
	[],
	{
		{
			call _x;
		} forEach KH_var_headlessMissionEndStack;

		call KH_fnc_headlessMissionEndInit;
	},
	"HEADLESS",
	true,
	false
];

triggerCbaEvent ["KH_eve_missionEnded", [_isVictory, _nextMission], "GLOBAL", false];

execute [
	[_endName, _isVictory, _fadeType, _nextMission],
	{
		params ["_endName", "_isVictory", "_fadeType", "_nextMission"];
		
		if (_nextMission isEqualTo []) then {
			execute [[_endName, _isVictory, _fadeType, true, false], "BIS_fnc_endMission", "GLOBAL", true, false];
		}
		else {
			_nextMission params [["_password", "", [""]], ["_mission", "", [""]]];
			_password serverCommand (["#mission ", _mission] joinString "");
		};
	},
	true,
	str _delay,
	false
];