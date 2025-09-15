params [["_unit", objNull, [objNull]]];

if (isPlayer _unit) then {
	_unit setVariable ["KH_var_disguiseState", false];
	_unit setVariable ["KH_var_disguiseDetected", true];
};

nil;