params ["_entity", ["_generateIfEmpty", true]];

if ((vehicleVarName _entity) isEqualTo "") then {
	if _generateIfEmpty then {
		[_entity, call KH_fnc_generateUid] call KH_fnc_setEntityVariableName;
	};
}
else {
	vehicleVarName _entity;
};