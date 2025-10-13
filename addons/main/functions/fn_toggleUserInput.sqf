params [["_input", userInputDisabled, [true]], ["_stop", true, [true]]];
		
if _input then {
	if userInputDisabled then {
		disableUserInput false;
	
		if _stop then {
			if (isNull (objectParent player)) then {
				player switchMove [""];
			};
		};
	};
}
else {
	if !userInputDisabled then {
		disableUserInput true;
		
		if _stop then {
			if (isNull (objectParent player)) then {
				player switchMove [""];
			};
		};
	};
};