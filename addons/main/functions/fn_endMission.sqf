params ["_endName", "_isVictory", ["_fadeType", true], ["_delay", 1], ["_nextMission", []]];
_delay = _delay max 1;
[] call KH_fnc_serverMissionEndInit;
[[], KH_fnc_headlessMissionEndInit, "HEADLESS", true] call KH_fnc_execute;
[[], KH_fnc_playerMissionEndInit, "PLAYERS", true] call KH_fnc_execute;
["KH_eve_missionEnded", [_isVictory, _nextMission]] call CBA_fnc_globalEvent;

[
	{
		params ["_endName", "_isVictory", "_fadeType", "_nextMission"];
		
		if (_nextMission isEqualTo []) then {
			[[_endName, _isVictory, _fadeType, true, false], "BIS_fnc_endMission", "GLOBAL", true] call KH_fnc_execute;
		}
		else {
			(_nextMission select 0) serverCommand (format ["#mission %1", _nextMission select 1]);
		};
	}, 
	[_endName, _isVictory, _fadeType, _nextMission],
	_delay
 ] call CBA_fnc_waitAndExecute;

 true;