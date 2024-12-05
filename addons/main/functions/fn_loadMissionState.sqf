params ["_missionState", ["_world", true], ["_variables", []]];
private _savedMissionState = [];

if (_missionState isEqualType []) then {
	_savedMissionState = _missionState;
}
else {
	_savedMissionState = missionProfileNamespace getVariable [format ["KH_var_savedMissionState_%1", _missionState], []];
};

_savedMissionState params ["_savedWorldInfo"];

if _world then {
	setDate (_savedWorldInfo select 0);
	0 setOvercast (_savedWorldInfo select 1);
	0 setFog (_savedWorldInfo select 2);
	0 setRain (_savedWorldInfo select 3);
	setWind (_savedWorldInfo select 4);
	0 setWindDir (_savedWorldInfo select 5);
};

if (_variables isNotEqualTo []) then {
	{
		private _variableName = _x;
		private _variableValue = missionProfileNamespace getVariable _x;
		missionNamespace setVariable [_variableName, _variableValue, true];
	} forEach _variables;
};

true;