params [["_unit", objNull, [objNull]]];

if (isPlayer _unit) then {
	[
		[_unit],
		{
			params ["_unit"];
			_unit setVariable ["KH_var_disguiseState", false];
			_unit setVariable ["KH_var_disguiseDetected", true];
		},
		"SERVER",
		true,
		false
	] call KH_fnc_execute;
};