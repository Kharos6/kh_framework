params ["_entity", ["_generateIfEmpty", true]];
private _variableName = vehicleVarName _entity;

if ((_variableName == "") && _generateIfEmpty) then {
	_variableName = [] call KH_fnc_generateUid;
	[_entity, _variableName] call KH_fnc_setEntityVariableName;
};

_variableName;