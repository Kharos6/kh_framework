params [["_unit", objNull, [objNull]]];

if (isPlayer _unit) then {
	_unit setVariable ["KH_var_disguiseState", false, 2];
	_unit setVariable ["KH_var_disguiseDetected", true, 2];
};

nil;