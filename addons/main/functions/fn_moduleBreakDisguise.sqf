isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		{
			if (isPlayer _x) then {
				_x setVariable ["KH_var_disguiseState", false];
				_x setVariable ["KH_var_disguiseDetected", true];
			};
		} forEach _units;
	};
};

true;