params ["_entity", ["_generateIfEmpty", true]];
private _variableName = vehicleVarName _entity;

if ((_variableName == "") && _generateIfEmpty) then {
	_variableName = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
	[_entity, _variableName] call KH_fnc_setEntityVariableName;
};

_variableName;