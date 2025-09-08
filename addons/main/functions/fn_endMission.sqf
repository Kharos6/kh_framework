params [["_endName", "", [""]], ["_isVictory", true, [true]], ["_fadeType", true, [true, 0]], ["_delay", 1, [0]], ["_nextMission", [], [[]]]];
_delay = _delay max 1;
call KH_fnc_serverMissionEndInit;
[[], "KH_fnc_playerMissionEndInit", "PLAYERS", true, false] call KH_fnc_execute;
[[], "KH_fnc_headlessMissionEndInit", "HEADLESS", true, false] call KH_fnc_execute;
["KH_eve_missionEnded", [_isVictory, _nextMission]] call CBA_fnc_globalEvent;

[
	[_endName, _isVictory, _fadeType, _nextMission],
	{
		params ["_endName", "_isVictory", "_fadeType", "_nextMission"];
		
		if (_nextMission isEqualTo []) then {
			[[_endName, _isVictory, _fadeType, true, false], "BIS_fnc_endMission", "GLOBAL", true, false] call KH_fnc_execute;
		}
		else {
			_nextMission params [["_password", "", [""]], ["_mission", "", [""]]];
			_password serverCommand (["#mission ", _mission] joinString "");
		};
	},
	true,
	str _delay,
	false
] call KH_fnc_execute;