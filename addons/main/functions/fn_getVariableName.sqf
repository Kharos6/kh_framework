params ["_entity", ["_string", true]];
private _variableName = vehicleVarName _entity;

if (_variableName == "") then {
	_variableName = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
	_entity setVehicleVarName _variableName;

	[
		[_entity, _variableName],
		{
			params ["_entity", "_variableName"];
			_entity setVehicleVarName _variableName;
		},
		["JIP", "GLOBAL", _entity, false, true, ""],
		"THIS_FRAME"
	] call KH_fnc_execute;
};

if _string then {
	_variableName;
}
else {
	_entity;
};