params ["_variableName", "_value"];

if !isMissionProfileNamespaceLoaded then {
	saveMissionProfileNamespace;
};

[
	{
		isMissionProfileNamespaceLoaded;
	}, 
	{
		params ["_variableName", "_value"];
		missionProfileNamespace setVariable [_variableName, _value];
		saveMissionProfileNamespace;
	}, 
	[_variableName, _value]
] call CBA_fnc_waitUntilAndExecute;

true;