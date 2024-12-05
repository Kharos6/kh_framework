params [["_input", userInputDisabled], ["_stop", true]];
		
if _input then {
	if userInputDisabled then {
		disableUserInput false;
	
		if _stop then {
			if (isNull (objectParent player)) then {
				[
					[player], 
					{
						params ["_unit"];
						_unit switchMove [""];
					},
					"GLOBAL",
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		};
	};
}
else {
	if !userInputDisabled then {
		disableUserInput true;
		
		if _stop then {
			if (isNull (objectParent player)) then {
				[
					[player], 
					{
						params ["_unit"];
						_unit switchMove [""];
					},
					"GLOBAL",
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		};
	};
};

true;