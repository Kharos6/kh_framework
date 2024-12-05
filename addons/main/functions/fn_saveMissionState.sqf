params ["_identifier", ["_variables", []]];
private _worldInfo = [date, overcast, fog, rain, wind, windDir];
private _stateEntries = [_worldInfo];

[
	[_identifier, _stateEntries, _variables], 
	{
		params ["_identifier", "_stateEntries", "_variables"];
		[format ["KH_var_savedMissionState_%1", _identifier], _stateEntries] call KH_fnc_editMissionProfileNamespace;

		if (_variables isNotEqualTo []) then {
			{
				[_x, missionNamespace getVariable _x] call KH_fnc_editMissionProfileNamespace;
			} forEach _variables;
		};
	},
	"GLOBAL",
	"THIS_FRAME"
] call KH_fnc_execute;

_stateEntries;